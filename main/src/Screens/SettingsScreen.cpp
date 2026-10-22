#include "SettingsScreen.h"

#include <Core/Application.h>
#include <Fonts/font.hpp>
#include <LV_Interface/LVGL.h>

#include "BLE/GAP.h"
#include "HomeScreen.h"
#include "PairingScreen.h"

static constexpr const char* TAG = "SettingsScreen";

SettingsScreen::SettingsScreen(){
	const auto app = Application::getApp();
	theme = app->getService<ThemeService>();
	buttonInput = app->getService<ButtonInput>();
	settings = app->getService<Settings>();
	com = app->getService<Com>();
	ledController = app->getService<LEDController>();

	if(com->getStatus() != Com::ConnStatus::Connected){
		app->getSingleton<BLE::GAP>()->disconnect();
	}

	theme->activateThemeAssets();

	buttonInput->OnButtonEvent.bind(app->getService<LVGL>(), [this](Enum<int> btn, ButtonInput::Action action) {
		handleButtonEvent((Button)(int)btn, action);
	});

	com->onConnStatus.bind(app->getService<LVGL>(), [this](const Com::ConnStatus status) {
		topBar->setRobotElementsVisible(status == Com::ConnStatus::Connected);
		if(status == Com::ConnStatus::Connected){
			ledController->wifiLedOn();
			ledController->bigGreenLedOff();
		}
	});

	com->onData.bind(app->getService<LVGL>(), [this](const BB::State state, const BB::Action action, const std::vector<uint8_t>& data) {
		handleDataEvent(state, action, data);
	});

	buildUI();
}

SettingsScreen::~SettingsScreen(){
	const auto app = Application::getApp();
	buttonInput->OnButtonEvent.unbind(app->getService<LVGL>());
	com->onConnStatus.unbind(app->getService<LVGL>());
	com->onData.unbind(app->getService<LVGL>());
}

void SettingsScreen::handleDataEvent(const BB::State state, const BB::Action action, const std::vector<uint8_t>& data){
	if(state != BB::State::Idle || action.idle != BB::Action::Idle::SettingsSync){
		return;
	}

	if(data.size() != sizeof(SettingsSyncData)){
		return;
	}

	const SettingsSyncData* sync = (const SettingsSyncData*)data.data();
	SettingsStruct currentSet = settings->get();
	currentSet.frontSensor = sync->frontSensor;
	currentSet.bottomSensor = sync->bottomSensor;
	currentSet.activityMode = sync->activityMode;
	settings->set(currentSet);

	if(settingsWindow != nullptr){
		settingsWindow->syncRobotSettings(currentSet);
	}
}

void SettingsScreen::handleButtonEvent(const Button btn, const ButtonInput::Action action){
	if (btn == Button::Joystick && action == ButtonInput::Action::Release){
		// Save current settings
		settings->store();

		if(com->getStatus() != Com::ConnStatus::Connected){
			transition([]() {
				return std::make_unique<PairingScreen>();
			});
			return;
		}

		const SettingsStruct currentSet = settings->get();
		com->sendSettings({ .frontSensor = currentSet.frontSensor, .bottomSensor = currentSet.bottomSensor, .activityMode = currentSet.activityMode });

		// Return to home screen
		transition([]() {
			return std::make_unique<HomeScreen>();
		});
	}
}

void SettingsScreen::loop(){
	topBar->loop();
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

void SettingsScreen::switchTheme(const Theme& newTheme){
	if (windowContainer != nullptr){
		theme->setTheme(newTheme);

		SettingsStruct currentSet = settings->get();
		currentSet.currentTheme = newTheme;
		settings->set(currentSet);

		lv_obj_clean(*this);
		buildUI();
	}
}

void SettingsScreen::buildUI(){
	const lv_color_t colorPrim = theme->getPrimaryColor();
	const lv_color_t bgColor = theme->getTertiaryColor();

	topBar = new TopBar(*this);
	topBar->setRobotElementsVisible(com->getStatus() == Com::ConnStatus::Connected);
	windowContainer = lv_obj_create(*this);

	settingsWindow = new SettingsWindow(windowContainer, inputGroup, [this](const Theme &newTheme) {
		switchTheme(newTheme);
	});

	// LABEL DEFAULT STYLE
	lv_style_set_text_font(labelDefaultStyle, &lv_font_butter);
	lv_style_set_text_line_space(labelDefaultStyle, 3);
	lv_style_set_text_color(labelDefaultStyle, colorPrim);
	lv_style_set_size(labelDefaultStyle, 80, LV_SIZE_CONTENT);
	lv_style_set_pad_all(labelDefaultStyle, 2);
	lv_style_set_bg_opa(labelDefaultStyle, LV_OPA_TRANSP);

	// Settings window container
	lv_obj_set_pos(windowContainer, 0, 8);
	lv_obj_set_style_pad_all(windowContainer, 4, 0);
	lv_obj_set_size(windowContainer, 128, 120);
	lv_obj_set_layout(windowContainer, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(windowContainer, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(windowContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

	lv_obj_set_style_bg_color(windowContainer, bgColor, 0);
	lv_obj_set_style_bg_opa(windowContainer, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_image_src(windowContainer, theme->getAsset(Asset::Grid), 0);

	// Joystick return text label
	lv_obj_t* joystickLabel = lv_label_create(*this);
	lv_obj_add_style(joystickLabel, labelDefaultStyle, 0);
	lv_label_set_text(joystickLabel, JoystickText);
	lv_obj_set_width(joystickLabel, 120);
	lv_obj_set_pos(joystickLabel, 6, 84);

	// Footer container
	lv_obj_t* footerContainer = lv_obj_create(*this);
	lv_obj_set_style_bg_color(footerContainer, bgColor, 0);
	lv_obj_set_size(footerContainer, 128, 31);
	lv_obj_set_pos(footerContainer, 0, 98);

	// Footer - top line
	lv_obj_t* footerTopLinie = lv_obj_create(footerContainer);
	lv_obj_set_style_bg_color(footerTopLinie, colorPrim, 0);
	lv_obj_set_size(footerTopLinie, 128, 1);
	lv_obj_set_pos(footerTopLinie, 0, 0);

	// Footer - TELEC image
	lv_obj_t* telecImg = lv_image_create(footerContainer);
	lv_image_set_src(telecImg, theme->getAsset(Asset::TELEC));
	lv_obj_set_pos(telecImg, 17, 3);

	// Footer - FCC text
	lv_obj_t* fccLabel = lv_label_create(footerContainer);
	lv_obj_add_style(fccLabel, labelDefaultStyle, 0);
	lv_label_set_text(fccLabel, FccText);
	lv_obj_set_width(fccLabel, 120);
	lv_obj_set_pos(fccLabel, 8, 18);
}
