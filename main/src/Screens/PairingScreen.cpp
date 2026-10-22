#include "PairingScreen.h"

#include <LV_Interface/LVGL.h>
#include <LV_Interface/LVGIF.h>

#include "HomeScreen.h"
#include "SettingsScreen.h"
#include "Services/ThemeService.h"
#include "Fonts/font.hpp"

PairingScreen::PairingScreen(){
	const auto app = Application::getApp();
	gap = app->getSingleton<BLE::GAP>();
	com = app->getService<Com>();
	ledController = app->getService<LEDController>();
	buttonInput = app->getService<ButtonInput>();

	// Blink Wifi LED
	ledController->wifiLedStrobe();
	ledController->bigGreenLedStrobe();

	// Load the pairing GIF archive
	app->getService<ThemeService>()->activatePairingAssets();

	lv_obj_set_style_bg_color(*this, lv_color_black(), 0);
	// Init pairing GIF
	LVGIF* pairingGif = new LVGIF(*this, "S:/pairing");
	pairingGif->setLooping(LVGIF::LoopType::On);
	pairingGif->reset();
	lv_obj_set_y(*pairingGif, -8);

	buildSettingsHint();

	gap->onConnEvent.bind(app->getService<LVGL>(), [this](const BLE::GAP::ConnEvent event) {
		// If connection failed, try again
		if(event == BLE::GAP::ConnEvent::Failed){
			gap->connect();
		}
	});

	com->onConnStatus.bind(app->getService<LVGL>(), [this](const Com::ConnStatus status) {
		// If connection successful, turn on LED and transition to home screen
		if(status == Com::ConnStatus::Connected){
			ledController->wifiLedOn();
			ledController->bigGreenLedOff();
			transition([]() {
				return std::make_unique<HomeScreen>();
			});
		}
	});

	buttonInput->OnButtonEvent.bind(app->getService<LVGL>(), [this](Enum<int> btn, ButtonInput::Action action) {
		handleButtonEvent(static_cast<Button>(static_cast<int>(btn)), action);
	});

	if(!gap->isConnected() && !gap->isConnecting()) gap->connect();
}

PairingScreen::~PairingScreen(){
	LVGL* lvgl = Application::getApp()->getService<LVGL>();

	gap->onConnEvent.unbind(lvgl);
	com->onConnStatus.unbind(lvgl);
	buttonInput->OnButtonEvent.unbind(lvgl);
}

void PairingScreen::buildSettingsHint(){
	// Pairing assets are theme-agnostic, so the hint always uses the default theme
	const ThemeStyle& style = ThemeService::getThemeStyle(Theme::Main);
	const ThemeService* theme = Application::getApp()->getService<ThemeService>();

	lv_obj_t* row = lv_obj_create(*this);
	lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_layout(row, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_ver(row, 1, 0);
	lv_obj_set_style_pad_hor(row, 2, 0);
	lv_obj_set_style_radius(row, 0, 0);
	lv_obj_set_style_border_width(row, 0, 0);
	lv_obj_set_style_bg_color(row, style.tertiaryColor, 0);
	lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
	lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, 0);

	lv_obj_t* label = lv_label_create(row);
	lv_obj_set_style_text_font(label, &lv_font_butter, 0);
	lv_obj_set_style_text_color(label, style.primaryColor, 0);
	lv_obj_set_style_pad_hor(label, 2, 0);
	lv_label_set_text_static(label, "Click joystick for");

	// The pairing archive carries the default theme's settings icon
	lv_obj_t* icon = lv_image_create(row);
	lv_image_set_src(icon, theme->getAsset(Asset::Settings));
	lv_obj_set_style_margin_all(icon, 1, 0);
}

void PairingScreen::handleButtonEvent(const Button btn, const ButtonInput::Action action){
	if(btn == Button::Joystick && action == ButtonInput::Action::Release){
		transition([]() {
			return std::make_unique<SettingsScreen>();
		});
	}
}

void PairingScreen::loop(){
	// (Re)connect whenever BLE is idle. A link can drop while it is still being set up - e.g. the controller was
	// restarted while connected and the robot still holds the previous link - and nothing else retries then.
	if(!gap->isConnected() && !gap->isConnecting()){
		gap->connect();
	}
}
