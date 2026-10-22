#ifndef BUTTERBOTCTRL_FIRMWARE_SETTINGSCHANGEWINDOW_H
#define BUTTERBOTCTRL_FIRMWARE_SETTINGSCHANGEWINDOW_H

#include "Components/HomeWindow.h"

class SettingsChangeWindow : public HomeWindow {
public:
	SettingsChangeWindow(lv_obj_t* parent, const SettingsChangeData* params);

	void onData(const BBData* data) override;

private:
	lv_obj_t* sentenceLabel;

	static constexpr const char* TITLE = "Settings";

	void buildUI(SettingsChangeData data);
};


#endif //BUTTERBOTCTRL_FIRMWARE_SETTINGSCHANGEWINDOW_H