#ifndef BUTTERBOTCTRL_FIRMWARE_SETTINGS_H
#define BUTTERBOTCTRL_FIRMWARE_SETTINGS_H

#include "Object/Object.h"
#include "Object/Class.h"
#include <nvs.h>
#include <CtrlData.h>

#include "Themes.hpp"

enum class InactivityTimeout : uint8_t {
	Min2 = 2, Min5 = 5, Min10 = 10, Min30 = 30, Off = 0xFF
};

struct SettingsStruct {
	float screenBrightness = 1.0f;
	Theme currentTheme = Theme::Main;
	InactivityTimeout inactivityTimeout = InactivityTimeout::Min10;
	FrontSensorMode frontSensor = FrontSensorMode::On;
	BottomSensorMode bottomSensor = BottomSensorMode::Reduced;
	ActivityMode activityMode = ActivityMode::Active;
};

class Settings : public Object {
	GENERATED_BODY(Settings, Object, void)
public:
	Settings();
	~Settings() override;

	SettingsStruct get() const;
	void set(const SettingsStruct& settings);
	void store() const;

private:
	SettingsStruct settingsStruct;

	static constexpr const char* BlobName = "Settings";
	static constexpr const char* NVSNamespace = "ButterbotCtrl";

	nvs_handle_t handle{};

	void load();
};


#endif //BUTTERBOTCTRL_FIRMWARE_SETTINGS_H
