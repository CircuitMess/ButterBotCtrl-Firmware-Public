#ifndef BUTTERBOTCTRL_FIRMWARE_SETTINGS_H
#define BUTTERBOTCTRL_FIRMWARE_SETTINGS_H

#include "Object/Object.h"
#include "Object/Class.h"
#include <nvs.h>

#include "Themes.hpp"
#include <CtrlData.h>

enum class InactivityTimeout : uint8_t {
	Min2 = 2, Min5 = 5, Min10 = 10, Min30 = 30, Off = 0xFF
};

// which robot proximity sensors are ignored. Stored separately from SettingsStruct
// so the existing settings blob stays compatible.
enum class SensorMode : uint8_t {
	AllOn = 0, FrontOff = 1, FloorOff = 2, AllOff = 3
};

inline Ctrl::Command sensorModeToCommand(SensorMode mode){
	switch(mode){
		case SensorMode::FrontOff: return Ctrl::SensorsFrontOff;
		case SensorMode::FloorOff: return Ctrl::SensorsFloorOff;
		case SensorMode::AllOff: return Ctrl::SensorsAllOff;
		default: return Ctrl::SensorsAllOn;
	}
}

struct SettingsStruct {
	float screenBrightness = 1.0f;
	Theme currentTheme = Theme::Main;
	InactivityTimeout inactivityTimeout = InactivityTimeout::Min10;
};

class Settings : public Object {
	GENERATED_BODY(Settings, Object, void)
public:
	Settings();
	~Settings() override;

	SettingsStruct get() const;
	void set(const SettingsStruct& settings);
	void store() const;

	SensorMode getSensorMode() const;
	void setSensorMode(SensorMode mode);

private:
	SettingsStruct settingsStruct;
	SensorMode sensorMode = SensorMode::AllOn;
	static constexpr const char* SensorKey = "Sensors";

	static constexpr const char* BlobName = "Settings";
	static constexpr const char* NVSNamespace = "ButterbotCtrl";

	nvs_handle_t handle{};

	void load();
};


#endif //BUTTERBOTCTRL_FIRMWARE_SETTINGS_H
