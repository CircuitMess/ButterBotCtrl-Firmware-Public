#ifndef BUTTERBOTCTRL_FIRMWARE_SETTINGSSCREEN_H
#define BUTTERBOTCTRL_FIRMWARE_SETTINGSSCREEN_H

#include <LV_Interface/LVScreen.h>
#include <Services/ButtonInput.h>

#include "Enums.hpp"
#include "Services/Com.h"
#include "Services/LEDController.h"
#include "Components/TopBar.h"
#include "Components/SettingsWindow.h"

class SettingsScreen : public LVScreen {
public:
	SettingsScreen();
	~SettingsScreen() override;

private:
	ThemeService* theme;
	Settings* settings;
	ButtonInput* buttonInput;
	Com* com;
	LEDController* ledController;

	lv_obj_t* windowContainer = nullptr;
	TopBar* topBar = nullptr;
	SettingsWindow* settingsWindow = nullptr;

	LVStyle labelDefaultStyle;

	static constexpr const char* JoystickText = "Press joystick to return";
	static constexpr const char* FccText = "FCC ID: 2AVZ4-BUTTER-BOT";

	void loop() override;

	void handleButtonEvent(Button btn, ButtonInput::Action action);
	void handleDataEvent(BB::State state, BB::Action action, const std::vector<uint8_t>& data);
	void buildUI();
	void switchTheme(const Theme &newTheme);
};

#endif //BUTTERBOTCTRL_FIRMWARE_SETTINGSSCREEN_H