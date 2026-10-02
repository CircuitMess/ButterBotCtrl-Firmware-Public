#ifndef BUTTERBOTCTRL_FIRMWARE_SETTINGSWINDOW_H
#define BUTTERBOTCTRL_FIRMWARE_SETTINGSWINDOW_H

#include <Enums.hpp>
#include <LV_Interface/LVObject.h>
#include <LV_Interface/LVStyle.h>

#include "Services/ThemeService.h"
#include <Services/Settings.h>
#include <Services/LED/LED.h>

#include "Components/SettingsWindow/BrightnessSlider.h"
#include "Components/SettingsWindow/ThemeSelector.h"
#include "Components/SettingsWindow/SleepSelector.h"
#include "Components/SettingsWindow/SensorSelector.h"

class SettingsWindow : public LVObject {
public:
	/**
	 * UI window that contains settings options
	 * @param parent parent LVGL object
	 * @param inputGroup parent input group
	 */
	SettingsWindow(lv_obj_t* parent, lv_group_t* inputGroup, const std::function<void(const Theme &newTheme)>& themeCb);
	~SettingsWindow() override;

	/* Main update loop, called by the parent */
	virtual void loop(){};

private:
	ThemeService* theme;
	Settings* settings;
	LED<LEDs, RGB_LEDs>* ledService;
	lv_group_t* inputGroup;

	ThemeSelector* themeSelector;
	SleepSelector* sleepSelector;
	BrightnessSlider* brightnessSlider;
	SensorSelector* sensorSelector;

	lv_obj_t* innerContent;
	LVStyle labelDefaultStyle;
	LVStyle barStyleBg;
	LVStyle barStyleIndic;
	LVStyle barStyleKnob;

	lv_anim_t blinkAnim;

	lv_obj_t* titleEl;
	lv_obj_t* titleLabel;
	lv_obj_t* corner;

	int32_t currentFocusIndex = 0;

	static constexpr int32_t WindowWidth = 120;
	static constexpr int32_t WindowHeight = 76;
	static constexpr int32_t RowCount = 4;
	static constexpr const char* WindowTitle = "Settings";

	const std::function<void(const Theme &newTheme)> themeCb;

	/* Properly scales window top bar with inner content */
	void updateLayout();
	void buildUI(lv_obj_t* parent);
};


#endif //BUTTERBOTCTRL_FIRMWARE_SETTINGSWINDOW_H