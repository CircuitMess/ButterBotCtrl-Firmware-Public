#include "HomeScreen.h"

#include <algorithm>
#include <Core/Application.h>
#include <Periphery/WiFi.h>
#include <LV_Interface/LVGL.h>
#include <LV_Interface/InputLVGL.h>

#include "PairingScreen.h"
#include "SettingsScreen.h"
#include "Fonts/font.hpp"
#include "Components/HomeWindows/RambleWindow.h"
#include "Components/HomeWindows/WanderWindow.h"
#include "Components/HomeWindows/PersonWindow.h"
#include "Components/HomeWindows/PokeWindow.h"
#include "Components/HomeWindows/SummonWindow.h"
#include "Components/HomeWindows/FallWindow.h"
#include "Components/HomeWindows/UpsideDownWindow.h"
#include "Components/HomeWindows/PickUpWindow.h"
#include "Components/HomeWindows/ShakeWindow.h"
#include "Components/HomeWindows/JokeWindow.h"
#include "Components/HomeWindows/FactWindow.h"
#include "Components/HomeWindows/PassTheButterWindow.h"
#include "Components/HomeWindows/YouPassButterWindow.h"
#include "Components/HomeWindows/ProfanityWindow.h"
#include "Components/HomeWindows/EightBallWindow.h"
#include "Components/HomeWindows/IR_trainWindow.h"
#include "Components/HomeWindows/IR_forgetWindow.h"
#include "Components/HomeWindows/IR_forgetAllWindow.h"
#include "Components/HomeWindows/IR_listWindow.h"
#include "Components/HomeWindows/IR_actionWindow.h"
#include "Components/HomeWindows/ListenWindow.h"
#include "Components/HomeWindows/DiceRollWindow.h"
#include "Components/HomeWindows/VoiceControlWindow.h"
#include "Components/HomeWindows/DanceWindow.h"
#include "Components/HomeWindows/CurrentTimeWindow.h"
#include "Components/HomeWindows/LEDModuleWindow.h"
#include "Components/HomeWindows/LEDTurnOnWindow.h"
#include "Components/HomeWindows/LEDTurnOffWindow.h"
#include "Components/HomeWindows/LEDStrobeWindow.h"
#include "Components/HomeWindows/LEDBreatheWindow.h"
#include "Components/HomeWindows/LEDFasterWindow.h"
#include "Components/HomeWindows/LEDSlowerWindow.h"
#include "Components/HomeWindows/TempHumModuleWindow.h"
#include "Components/HomeWindows/TempHumScaleWindow.h"
#include "Components/HomeWindows/GasModuleWindow.h"
#include "Components/HomeWindows/GasConfigStartWindow.h"
#include "Components/HomeWindows/GasConfigEndWindow.h"
#include "Components/HomeWindows/ModuleChangeWindow.h"
#include "Components/HomeWindows/FaceDetectWindow.h"
#include "Components/HomeWindows/FaceForgetWindow.h"
#include "Components/HomeWindows/IntruderWindow.h"
#include "Components/HomeWindows/IntruderDetectionWindow.h"
#include "Components/HomeWindows/PhoneConnectWindow.h"
#include "Components/HomeWindows/PhoneNotConnectedWindow.h"
#include "Components/HomeWindows/PhoneListNotifsWindow.h"
#include "Components/HomeWindows/WhatsPlayingWindow.h"
#include "Components/HomeWindows/MediaControlWindow.h"
#include "Components/HomeWindows/ShutdownWindow.h"
#include "Components/HomeWindows/BatteryLowWindow.h"
#include "Components/HomeWindows/ChargingWindow.h"
#include "Components/HomeWindows/ChargingFullWindow.h"
#include "Components/HomeWindows/WhatsThisWindow.h"
#include "Components/HomeWindows/ObserveWindow.h"
#include "Components/HomeWindows/CantMoveWindow.h"
#include "Components/HomeWindows/SettingsChangeWindow.h"
#include "Components/HomeWindows/HoldPopupWindow.h"
#include "RCScreen.h"
#include "Util/WiFiAccessPoint.h"

static constexpr const char* TAG = "HomeScreen";

HomeScreen::HomeScreen(){
	const auto app = Application::getApp();
	com = app->getService<Com>();
	theme = app->getService<ThemeService>();
	buttonInput = app->getService<ButtonInput>();
	ledController = app->getService<LEDController>();
	robotState = app->getService<RobotState>();

	// Load the theme archive
	theme->activateThemeAssets();

	topBar = new TopBar(*this);
	windowContainer = lv_obj_create(*this);
	scrollOffset = lv_obj_create(windowContainer);

	com->onData.bind(app->getService<LVGL>(), [this](const BB::State state, const BB::Action action, const std::vector<uint8_t>& data) {
		ledDataBlink(data);
		handleDataEvent(state, action, data);
	});

	com->onConnStatus.bind(app->getService<LVGL>(), [this](const Com::ConnStatus event) {
		// Return to pairing screen if connection lost
		if(event == Com::ConnStatus::Disconnected){
			transition([]() {
				return std::make_unique<PairingScreen>();
			});
		}
	});

	buttonInput->OnButtonEvent.bind(app->getService<LVGL>(), [this](Enum<int> btn, ButtonInput::Action action) {
		handleButtonEvent(static_cast<Button>(static_cast<int>(btn)), action);
	});

	if(WiFiAccessPoint* accessPoint = app->getPeriphery<WiFiAccessPoint>()){
		LVGL* lvgl = app->getService<LVGL>();

		accessPoint->OnConnectionEvent.bind(lvgl, [this](WiFiAccessPoint::EventType type) {
			if(type == WiFiAccessPoint::EventType::Connect){
				rcWaitStartTime = 0;
				transition([]() {
					return std::make_unique<RCScreen>();
				});
			}
		});
	}

	buildUI();
}

HomeScreen::~HomeScreen(){
	LVGL* lvgl = Object::getApp()->getService<LVGL>();

	lv_anim_delete(activeWindow, nullptr);
	com->onData.unbind(lvgl);
	com->onConnStatus.unbind(lvgl);
	buttonInput->OnButtonEvent.unbind(lvgl);

	if(WiFiAccessPoint* accessPoint = Object::getApp()->getPeriphery<WiFiAccessPoint>()){
		accessPoint->OnConnectionEvent.unbind(lvgl);
	}
}

void HomeScreen::handleButtonEvent(const Button btn, const ButtonInput::Action action){
	// All input is disabled while waiting for the RC feed to start
	if(rcWaitStartTime != 0){
		return;
	}

	// Action list is modal: the joystick is routed through the LVGL keypad indev
	// (up/down/enter), any other button press dismisses the list and is swallowed
	if(actionElement != nullptr){
		if(btn != Button::Joystick && action == ButtonInput::Action::Press){
			clearActions();
			actionDismissBtn = btn;
		}
		return;
	}

	// The dismissing button's release must not act either
	if(btn == actionDismissBtn && action == ButtonInput::Action::Release){
		actionDismissBtn = Button::COUNT;
		return;
	}

	// Early return if popup modal active
	if(popup != nullptr && btn != popupOwner){
		if(static_cast<HoldPopupWindow*>(popup)->isBarFull() || overridePressTime != 0 || summonPressTime != 0){
			return;
		}
	}

	if(btn == Button::Joystick && action == ButtonInput::Action::Release){
		showActions();
		return;
	}

	if(action == ButtonInput::Action::Press){
		if(btn == Button::ShutUp){
			com->sendCommand(Ctrl::Command::ShutUp);
		} else if(btn == Button::Summon){
			// Summon press takes over the short-press override hint
			if(overrideHintShownAt != 0){
				clearPopupFor(Button::ManualOverride);
			}
			summonPressTime = millis();
			// Hold is ignored while a window is on screen; short press still works
			summonHoldEnabled = activeWindow == nullptr;
		} else if(btn == Button::ManualOverride){
			// Manual override (RC) can only be initiated while the robot is in Idle::None
			if(robotState == nullptr || !robotState->isIdleNone()){
				return;
			}

			overridePressTime = millis();
			showPopup(new HoldPopupWindow(*this, "Connecting"), Button::ManualOverride);
		}
		return;
	}

	if(btn == Button::Poke){
		com->sendCommand(Ctrl::Command::Poke);
	} else if(btn == Button::Summon){
		if(summonPressTime != 0 && millis() - summonPressTime < SummonShortPressMaxMs && listenSendCount < MaxListenSends){
			com->sendCommand(Ctrl::Command::Listen);
			listenSendCount++;
		}
		summonPressTime = 0;
		clearPopupFor(Button::Summon);
	} else if(btn == Button::ManualOverride){
		if(overridePressTime != 0 && millis() - overridePressTime <= OverrideShortPressMaxMs &&
			popup != nullptr && popupOwner == Button::ManualOverride){
			// Short press: swap the fill bar for a hint, unless a window is already shown underneath
			if(activeWindow != nullptr){
				clearPopupFor(Button::ManualOverride);
			} else{
				static_cast<HoldPopupWindow*>(popup)->showHint("Hold for RC");
				overrideHintShownAt = millis();
			}
		} else{
			clearPopupFor(Button::ManualOverride);
		}
		overridePressTime = 0;
	}
}

void HomeScreen::ledDataBlink(const std::vector<uint8_t>& data){
	const uint32_t blinkCount = std::max<uint32_t>(1, data.size() / 10);
	ledController->bigGreenLedStrobeData(blinkCount);
}

void HomeScreen::loop(){
	// A scenario selection requested the list be closed from inside its event callback
	if(actionsClosePending){
		clearActions();
	}

	topBar->loop();
	if(activeWindow != nullptr) activeWindow->loop();
	if(popup != nullptr) popup->loop();

	processQueue();

	// Give up on the RC feed if it hasn't started in time
	if(rcWaitStartTime != 0 && millis() - rcWaitStartTime >= RCLoadTimeoutMs){
		rcWaitStartTime = 0;
	}
	lv_obj_update_flag(rcLoadingLabel, LV_OBJ_FLAG_HIDDEN, rcWaitStartTime == 0);

	// Show the guide only when nothing else is on screen
	const bool busy = activeWindow != nullptr || animActive || !windowQueue.empty() || clearPending || popup != nullptr ||
		actionElement != nullptr || rcWaitStartTime != 0;
	lv_obj_update_flag(*guide, LV_OBJ_FLAG_HIDDEN, busy);

	// Show hold popup if btn held past the short-press window
	if(summonHoldEnabled && summonPressTime != 0 && popup == nullptr && millis() - summonPressTime >= SummonShortPressMaxMs){
		showPopup(new HoldPopupWindow(*this, "Summon", SummonLongPressMinMs - SummonShortPressMaxMs), Button::Summon);
	}

	if(summonHoldEnabled && summonPressTime != 0 && millis() - summonPressTime >= SummonLongPressMinMs){
		com->sendCommand(Ctrl::Command::Summon);
		summonPressTime = 0;
	}

	if(overridePressTime != 0 && millis() - overridePressTime >= OverrideHoldMinMs){
		overridePressTime = 0;
		startRC();
	}

	if(overrideHintShownAt != 0 && millis() - overrideHintShownAt >= OverrideHintDurationMs){
		clearPopupFor(Button::ManualOverride);
	}
}

void HomeScreen::startRC(){
	WiFiAccessPoint* accessPoint = Object::getApp()->getPeriphery<WiFiAccessPoint>();
	if(accessPoint == nullptr){
		return;
	}

	if(com != nullptr){
		RCData data{};

		memcpy(data.SSID, accessPoint->getSSID(), sizeof(RCData::SSID));
		memcpy(data.password, accessPoint->getPassword(), sizeof(RCData::password));

		com->sendRC(data);
		rcWaitStartTime = millis();

		// Button events are ignored while waiting for RC, manually clear popup
		clearPopup();
		summonPressTime = 0;
	}
}

void HomeScreen::handleDataEvent(const BB::State state, const BB::Action action, const std::vector<uint8_t>& data){
	WiFiAccessPoint* accessPoint = Object::getApp()->getPeriphery<WiFiAccessPoint>();

	if(state == BB::State::RC){
		return;
	}

	if(state == BB::State::Idle && action.idle == BB::Action::Idle::None){
		listenSendCount = 0;
	}

	if(accessPoint != nullptr && accessPoint->getConnectionStatus() == WiFiAccessPoint::EventType::Connect){
		if(state == BB::State::Idle && action.idle == BB::Action::Idle::None){
			clearWindow();
		} else{
			// Create new window
			HomeWindow* window = createActionWindow(state, action, data);
			if(window == nullptr){
				ESP_LOGE(TAG, "OnData Window creation failed");
				return;
			}
			insertWindow(window, state == BB::State::Listen);
		}

		return;
	}

	// Battery level packets are consumed by RobotState/TopBar; not a window action.
	if(state == BB::State::Idle && action.idle == BB::Action::Idle::BatteryLevel){
		return;
	}

	if(state == BB::State::Idle && action.idle == BB::Action::Idle::SettingsSync){
		return;
	}

	// Listen carries its phase in the data payload; a phase change means a new window,
	// a repeated packet of the same phase is only an update
	bool listenPhaseChanged = false;
	if(state == BB::State::Listen && data.size() == sizeof(ListenData)){
		const auto phase = ((const ListenData*) data.data())->phase;
		listenPhaseChanged = phase != lastListenPhase;
		lastListenPhase = phase;
	}

	// Check if state or action changed
	if(state != comState.state ||
		(state == BB::State::Idle && comState.action.idle != action.idle) ||
		(state == BB::State::Scenario && comState.action.scenario != action.scenario) ||
		(state == BB::State::Listen && listenPhaseChanged)){
		// Enqueue new action
		if(windowQueue.size() >= MaxQueuedWindows){
			// Drop the oldest unshown event so the display favors recent activity
			windowQueue.pop();
			ESP_LOGW(TAG, "Window queue full, dropping oldest event");
		}
		windowQueue.push({ state, action, data });
	} else if(!windowQueue.empty()){
		// Update for already enqueued action
		windowQueue.back().data = data;
	} else if(activeWindow != nullptr){
		// Update active window
		if(state == BB::State::Scenario && action.scenario == BB::Action::Scenario::PhoneWhatsPlaying){
			// WhatsPlayingData is variable-length (std::string fields); deserialize into a real object
			const WhatsPlayingData wp = WhatsPlayingData::deserialize(data.data(), data.size());
			activeWindow->onData(&wp);
		} else{
			const auto params = (BBData*)data.data();
			activeWindow->onData(params);
		}
		activeWindowUpdatedAt = millis();
	}

	// Update current state
	comState = { state, action };
}

template<typename T_Window, typename T_Data>
T_Window* initActionWindow(lv_obj_t* parent, const std::vector<uint8_t>& data){
	if(data.size() != sizeof(T_Data)){
		ESP_LOGE(TAG, "Wrong data size for type of action");
		return nullptr;
	}
	auto params = (T_Data*)data.data();
	return new T_Window(parent, params);
}

HomeWindow* HomeScreen::createActionWindow(const BB::State state, const BB::Action action, const std::vector<uint8_t>& data){
	HomeWindow* newWin = nullptr;
	if(state == BB::State::Listen){
		newWin = initActionWindow<ListenWindow, ListenData>(windowContainer, data);
	} else if(state == BB::State::Scenario){
		switch(action.scenario){
			case BB::Action::Scenario::EightBall:
				newWin = initActionWindow<EightBallWindow, EightBallData>(windowContainer, data);
				break;
			case BB::Action::Scenario::IR_train:
				newWin = initActionWindow<IR_trainWindow, IR_trainData>(windowContainer, data);
				break;
			case BB::Action::Scenario::IR_forget:
				newWin = initActionWindow<IR_forgetWindow, IR_forgetData>(windowContainer, data);
				break;
			case BB::Action::Scenario::IR_forgetAll:
				newWin = initActionWindow<IR_forgetAllWindow, IR_forgetAllData>(windowContainer, data);
				break;
			case BB::Action::Scenario::IR_list:
				newWin = initActionWindow<IR_listWindow, IR_listData>(windowContainer, data);
				break;
			case BB::Action::Scenario::IR_action:
				newWin = initActionWindow<IR_actionWindow, IR_actionData>(windowContainer, data);
				break;
			case BB::Action::Scenario::Joke:
				newWin = initActionWindow<JokeWindow, JokeData>(windowContainer, data);
				break;
			case BB::Action::Scenario::Fact:
				newWin = initActionWindow<FactWindow, FactData>(windowContainer, data);
				break;
			case BB::Action::Scenario::PassTheButter:
				newWin = initActionWindow<PassTheButterWindow, PassTheButterData>(windowContainer, data);
				break;
			case BB::Action::Scenario::YouPassButter:
				newWin = initActionWindow<YouPassButterWindow, YouPassButterData>(windowContainer, data);
				break;
			case BB::Action::Scenario::Profanity:
				newWin = initActionWindow<ProfanityWindow, ProfanityData>(windowContainer, data);
				break;
			case BB::Action::Scenario::CurrentTime:
				newWin = initActionWindow<CurrentTimeWindow, CurrentTimeData>(windowContainer, data);
				break;
			case BB::Action::Scenario::Dance:
				newWin = initActionWindow<DanceWindow, DanceData>(windowContainer, data);
				break;
			case BB::Action::Scenario::VoiceControl:
				newWin = initActionWindow<VoiceControlWindow, VoiceControlData>(windowContainer, data);
				break;
			case BB::Action::Scenario::DiceRoll:
				newWin = initActionWindow<DiceRollWindow, DiceRollData>(windowContainer, data);
				break;
			case BB::Action::Scenario::LEDModule: // LED Module inserted
				newWin = initActionWindow<LEDModuleWindow, LEDModuleData>(windowContainer, data);
				break;
			case BB::Action::Scenario::LEDTurnOn:
				newWin = initActionWindow<LEDTurnOnWindow, LEDTurnOnData>(windowContainer, data);
				break;
			case BB::Action::Scenario::LEDTurnOff:
				newWin = initActionWindow<LEDTurnOffWindow, LEDTurnOffData>(windowContainer, data);
				break;
			case BB::Action::Scenario::LEDStrobe:
				newWin = initActionWindow<LEDStrobeWindow, LEDStrobeData>(windowContainer, data);
				break;
			case BB::Action::Scenario::LEDBreathe:
				newWin = initActionWindow<LEDBreatheWindow, LEDBreatheData>(windowContainer, data);
				break;
			case BB::Action::Scenario::LEDFaster:
				newWin = initActionWindow<LEDFasterWindow, LEDFasterData>(windowContainer, data);
				break;
			case BB::Action::Scenario::LEDSlower:
				newWin = initActionWindow<LEDSlowerWindow, LEDSlowerData>(windowContainer, data);
				break;
			case BB::Action::Scenario::TempHumModule:
				newWin = initActionWindow<TempHumModuleWindow, TempHumModuleData>(windowContainer, data);
				break;
			case BB::Action::Scenario::TempHumScaleCelsius:
			case BB::Action::Scenario::TempHumScaleFahrenheit:
			case BB::Action::Scenario::TempHumScaleKelvin:
				newWin = initActionWindow<TempHumScaleWindow, TempHumScaleData>(windowContainer, data);
				break;
			case BB::Action::Scenario::FaceDetect:
				newWin = initActionWindow<FaceDetectWindow, FaceDetectData>(windowContainer, data);
				break;
			case BB::Action::Scenario::FaceDetectForget:
				newWin = initActionWindow<FaceForgetWindow, FaceForgetData>(windowContainer, data);
				break;
			case BB::Action::Scenario::IntruderDetectionOn:
				newWin = initActionWindow<IntruderDetectionOnWindow, PIRModuleData>(windowContainer, data);
				break;
			case BB::Action::Scenario::IntruderDetectionOff:
				newWin = initActionWindow<IntruderDetectionOffWindow, PIRModuleData>(windowContainer, data);
				break;
			case BB::Action::Scenario::GasModule:
				newWin = initActionWindow<GasModuleWindow, GasData>(windowContainer, data);
				break;
			case BB::Action::Scenario::PhoneNotConnected:
				newWin = initActionWindow<PhoneNotConnectedWindow, PhoneNotConnectedData>(windowContainer, data);
				break;
			case BB::Action::Scenario::PhoneListNotifs:
				newWin = initActionWindow<PhoneListNotifsWindow, PhoneListNotifsData>(windowContainer, data);
				break;
			case BB::Action::Scenario::PhoneWhatsPlaying: {
				// WhatsPlayingData is variable-length (std::string fields); deserialize it explicitly
				const WhatsPlayingData wpData = WhatsPlayingData::deserialize(data.data(), data.size());
				newWin = new WhatsPlayingWindow(windowContainer, &wpData);
				break;
			}
			case BB::Action::Scenario::PhonePrevSong:
				newWin = initActionWindow<PrevSongWindow, MediaControlData>(windowContainer, data);
				break;
			case BB::Action::Scenario::PhoneNextSong:
				newWin = initActionWindow<NextSongWindow, MediaControlData>(windowContainer, data);
				break;
			case BB::Action::Scenario::PhonePlayMusic:
				newWin = initActionWindow<PlayMusicWindow, MediaControlData>(windowContainer, data);
				break;
			case BB::Action::Scenario::PhoneStopMusic:
				newWin = initActionWindow<StopMusicWindow, MediaControlData>(windowContainer, data);
				break;
			case BB::Action::Scenario::Shutdown:
				newWin = initActionWindow<ShutdownWindow, ShutdownData>(windowContainer, data);
				break;
			case BB::Action::Scenario::WhatsThis: // Object Detection window - on demand
				newWin = initActionWindow<WhatsThisWindow, WhatsThisData>(windowContainer, data);
				break;
			case BB::Action::Scenario::SettingsActivity:
				newWin = initActionWindow<SettingsChangeWindow, SettingsChangeData>(windowContainer, data);
				break;
			default:
				ESP_LOGE(TAG, "Action scenario not recognised");
				break;
		}
	} else{
		switch(action.idle){
			case BB::Action::Idle::None:
				ESP_LOGE(TAG, "createActionWindow got Idle::None!");
				// clearWindow();
				break;
			case BB::Action::Idle::Person:
				newWin = initActionWindow<PersonWindow, PersonData>(windowContainer, data);
				break;
			case BB::Action::Idle::Wander:
				newWin = initActionWindow<WanderWindow, WanderData>(windowContainer, data);
				break;
			case BB::Action::Idle::Ramble:
				newWin = initActionWindow<RambleWindow, RambleData>(windowContainer, data);
				break;
			case BB::Action::Idle::Poke:
				newWin = initActionWindow<PokeWindow, PokeData>(windowContainer, data);
				break;
			case BB::Action::Idle::Summon: // NOT DONE
				newWin = initActionWindow<SummonWindow, SummonData>(windowContainer, data);
				break;
			case BB::Action::Idle::Fall:
				newWin = initActionWindow<FallWindow, FallData>(windowContainer, data);
				break;
			case BB::Action::Idle::UpsideDown:
				newWin = initActionWindow<UpsideDownWindow, UpsideDownData>(windowContainer, data);
				break;
			case BB::Action::Idle::PickUp:
				newWin = initActionWindow<PickUpWindow, PickUpData>(windowContainer, data);
				break;
			case BB::Action::Idle::Shake:
				newWin = initActionWindow<ShakeWindow, ShakeData>(windowContainer, data);
				break;
			case BB::Action::Idle::ModuleChange:
				newWin = initActionWindow<ModuleChangeWindow, ModuleData>(windowContainer, data);
				break;
			case BB::Action::Idle::Intruder:
				newWin = initActionWindow<IntruderWindow, IntruderData>(windowContainer, data);
				break;
			case BB::Action::Idle::PhoneConnect:
				newWin = initActionWindow<PhoneConnectWindow, PhoneConnData>(windowContainer, data);
				break;
			case BB::Action::Idle::BatteryLow:
				newWin = initActionWindow<BatteryLowWindow, BatteryData>(windowContainer, data);
				break;
			case BB::Action::Idle::Charging:
				newWin = initActionWindow<ChargingWindow, BatteryData>(windowContainer, data);
				break;
			case BB::Action::Idle::ChargingFull:
				newWin = initActionWindow<ChargingFullWindow, BatteryData>(windowContainer, data);
				break;
			case BB::Action::Idle::Observe: // Object Detection window - random/pasivni
				newWin = initActionWindow<ObserveWindow, ObserveData>(windowContainer, data);
				break;
			case BB::Action::Idle::GasConfigureStart:
				newWin = initActionWindow<GasConfigStartWindow, GasConfigureStartData>(windowContainer, data);
				break;
			case BB::Action::Idle::GasConfigureEnd:
				newWin = initActionWindow<GasConfigEndWindow, GasConfigureEndData>(windowContainer, data);
				break;
			case BB::Action::Idle::Gas:
				newWin = initActionWindow<GasModuleWindow, GasData>(windowContainer, data);
				break;
			case BB::Action::Idle::CantMove:
				newWin = initActionWindow<CantMoveWindow, CantMoveData>(windowContainer, data);
				break;
			case BB::Action::Idle::SettingsChange:
				newWin = initActionWindow<SettingsChangeWindow, SettingsChangeData>(windowContainer, data);
				break;
			default:
				ESP_LOGE(TAG, "Idle action not recognised");
				break;
		}
	}
	return newWin;
}

void HomeScreen::insertWindow(HomeWindow* window, const bool isListen){
	// A new window replaces the short-press override hint
	if(overrideHintShownAt != 0){
		clearPopupFor(Button::ManualOverride);
	}

	// The outgoing ListenWindow re-enables the summon short press
	if(activeWindow != nullptr && activeWindowIsListen){
		listenSendCount = 0;
	}
	activeWindowIsListen = isListen;

	// Only ever called by processQueue() while !animActive, so the swap is always safe.
	lv_obj_move_to_index(*window, 0);
	if(activeWindow != nullptr){
		// Replace: fade the old window out and scroll-in new one
		activeWindow->onDismiss();

		lv_anim_set_var(&windowFadeAnim, activeWindow);
		animActive = true;

		lv_obj_update_layout(windowContainer);
		const int32_t winH = lv_obj_get_height(*window) + WindowGap;

		lv_obj_scroll_to_y(windowContainer, winH, LV_ANIM_OFF);

		lv_anim_start(&windowFadeAnim);
		lv_obj_scroll_to_y(windowContainer, 0, LV_ANIM_ON);
	} else{
		// First window: nothing to fade, just slide-in
		lv_obj_update_layout(windowContainer);
		const int32_t winH = lv_obj_get_height(*window) + WindowGap;

		lv_obj_scroll_to_y(windowContainer, winH, LV_ANIM_OFF);
		lv_obj_scroll_to_y(windowContainer, 0, LV_ANIM_ON);
	}
	activeWindow = window;
	updateAnim();
}

void HomeScreen::showPopup(HomeWindow* window, const Button owner){
	if(window == nullptr){
		ESP_LOGE(TAG, "Popup creation failed");
		return;
	}

	// Interrupt any pop-up already on screen
	clearPopup();

	popup = window;
	popupOwner = owner;

	lv_obj_update_layout(*popup);
	lv_obj_center(*popup);
	lv_obj_move_foreground(*popup);
}

void HomeScreen::clearPopup(){
	if(popup != nullptr){
		delete popup;
		popup = nullptr;
		popupOwner = Button::COUNT;
	}
	overrideHintShownAt = 0;
}

void HomeScreen::clearPopupFor(const Button owner){
	if(popup != nullptr && popupOwner == owner){
		clearPopup();
	}
}

void HomeScreen::showActions(){
	if(actionElement != nullptr) return;

	// Cancel any in-flight hold so loop() can't spawn a HoldPopupWindow over the list
	clearPopup();
	summonPressTime = 0;
	overridePressTime = 0;

	actionElement = new ActionElement(*this, inputGroup, [this]() {
		// SETTINGS entry; the list is cleaned up by the screen transition
		transition([]() {
			return std::make_unique<SettingsScreen>();
		});
	}, [this](const BB::Action::Scenario scenario, const ScenarioData data) {
		com->sendScenario(scenario, data);
		// Deleting the list mid-event-dispatch is unsafe; close it on the next loop() tick
		actionsClosePending = true;
	}, lastActionIndex);

	lv_obj_set_pos(*actionElement, 0, 8); // right below the TopBar
	lv_obj_move_foreground(*actionElement);

	// Swallow any in-flight ENTER: the joystick release that opened the list must not
	// register as a press on the freshly focused button
	lv_indev_wait_release(InputLVGL::getInstance()->getIndev());
}

void HomeScreen::clearActions(){
	actionsClosePending = false;
	if(actionElement == nullptr) return;

	lastActionIndex = actionElement->getSelectedIndex();
	delete actionElement; // lv_obj deletion auto-removes the buttons from inputGroup
	actionElement = nullptr;
}

void HomeScreen::clearWindow(){
	// Check if there is active window
	if(activeWindow != nullptr){
		// A removed ListenWindow re-enables the summon short press
		if(activeWindowIsListen){
			listenSendCount = 0;
			activeWindowIsListen = false;
		}

		lv_anim_set_var(&windowFadeAnim, activeWindow);
		animActive = true;
		lv_anim_start(&windowFadeAnim);
		activeWindow = nullptr;
	}
}

void HomeScreen::processQueue(){
	if(animActive) return; // wait for the fade animation to finish

	// A deferred clear from an Idle::None; Hold the window for a grace period
	// New action arriving cancels the fade and slides in over the current window
	if(clearPending){
		if(!windowQueue.empty()){
			// A new action is waiting — cancel the fade and replace with slide-in
			clearPending = false;
		} else if(millis() - clearPendingAt >= ClearGracePeriodMs &&
			(activeWindow == nullptr || millis() - activeWindowUpdatedAt >= UninterruptedDisplayMs)){
			// Grace period elapsed with nothing queued, and the window has been on
			// screen long enough — fade it out
			clearPending = false;
			clearWindow();
			return;
		} else{
			// Keep the current window visible while we wait out the grace period
			return;
		}
	}

	if(windowQueue.empty()) return;
	if(activeWindow != nullptr && (millis() - activeWindowShownAt) < MinWindowDisplayMs) return;

	PendingEvent nextEvent = std::move(windowQueue.front());
	windowQueue.pop();

	// Idle::None doesn't clear immediately; defer it so a quickly-following scenario can
	// slide in over this window instead of fading out
	if(nextEvent.state == BB::State::Idle && nextEvent.action.idle == BB::Action::Idle::None){
		clearPending = true;
		clearPendingAt = millis();
		return;
	}

	HomeWindow* window = createActionWindow(nextEvent.state, nextEvent.action, nextEvent.data);
	if(window == nullptr){
		ESP_LOGE(TAG, "OnData Window creation failed");
		return;
	}
	insertWindow(window, nextEvent.state == BB::State::Listen);
	activeWindowShownAt = millis();
	activeWindowUpdatedAt = activeWindowShownAt;
}

void HomeScreen::buildUI(){
	const lv_color_t bgColor = theme->getTertiaryColor();

	lv_obj_set_pos(windowContainer, 0, 8);
	lv_obj_set_style_pad_all(windowContainer, 4, 0);
	lv_obj_set_size(windowContainer, 128, 120);
	lv_obj_set_layout(windowContainer, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(windowContainer, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(windowContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(windowContainer, WindowGap, 0);

	lv_obj_set_style_bg_color(windowContainer, bgColor, 0);
	lv_obj_set_scrollbar_mode(windowContainer, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_style_bg_opa(windowContainer, LV_OPA_COVER, 0);
	lv_obj_add_flag(windowContainer, LV_OBJ_FLAG_SCROLLABLE);

	// Scroll offset
	lv_obj_set_size(scrollOffset, 1, 200);
	lv_obj_set_style_opa(scrollOffset, LV_OPA_TRANSP, 0);

	// Guide element
	guide = new GuideElement(windowContainer);
	lv_obj_center(*guide);

	// "Loading RC..." label
	rcLoadingLabel = lv_label_create(windowContainer);
	lv_obj_add_flag(rcLoadingLabel, LV_OBJ_FLAG_IGNORE_LAYOUT);
	lv_obj_add_flag(rcLoadingLabel, LV_OBJ_FLAG_FLOATING);
	lv_obj_add_flag(rcLoadingLabel, LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text_static(rcLoadingLabel, RCLoadText);
	lv_obj_set_style_text_font(rcLoadingLabel, &lv_font_butter, 0);
	lv_obj_set_style_text_color(rcLoadingLabel, theme->getPrimaryColor(), 0);
	lv_obj_set_style_text_align(rcLoadingLabel, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_center(rcLoadingLabel);

	// Prepare window fade animation
	lv_anim_init(&windowFadeAnim);
	lv_anim_set_duration(&windowFadeAnim, 500);
	lv_anim_set_values(&windowFadeAnim, LV_OPA_COVER, LV_OPA_0);
	lv_anim_set_exec_cb(&windowFadeAnim, [](void* obj, const int32_t var) {
		lv_obj_set_style_opa(*(HomeWindow*)obj, var, 0);
	});
	lv_anim_set_completed_cb(&windowFadeAnim, [](lv_anim_t* anim) {
		const auto animData = (AnimUserData*)lv_anim_get_user_data(anim);
		*animData->animActive = false;
		delete (HomeWindow*)anim->var;
	});
	updateAnim();
}

void HomeScreen::updateAnim(){
	if(activeWindow != nullptr){
		currentAnimData = {
			.animActive = &animActive,
		};
		lv_anim_set_user_data(&windowFadeAnim, &currentAnimData);
	}
}
