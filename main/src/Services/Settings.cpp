#include "Settings.h"
#include <nvs_flash.h>

DEFINE_LOG(Settings)

Settings::Settings(){
	ESP_ERROR_CHECK(nvs_open(NVSNamespace, NVS_READWRITE, &handle));
	load();
}

Settings::~Settings(){
	nvs_close(handle);
}

SettingsStruct Settings::get() const{
	return settingsStruct;
}

void Settings::set(const SettingsStruct& settings){
	settingsStruct = settings;
}

void Settings::store() const{
	esp_err_t err = nvs_set_blob(handle, BlobName, &settingsStruct, sizeof(SettingsStruct));
	if(err != ESP_OK){
		CMF_LOG(Settings, LogLevel::Error, "Error storing settings: %s", esp_err_to_name(err));
		return;
	}
	err = nvs_set_u8(handle, SensorKey, static_cast<uint8_t>(sensorMode));
	if(err != ESP_OK){
		CMF_LOG(Settings, LogLevel::Error, "Error storing sensor mode: %s", esp_err_to_name(err));
	}
	nvs_commit(handle);
}

SensorMode Settings::getSensorMode() const{
	return sensorMode;
}

void Settings::setSensorMode(SensorMode mode){
	sensorMode = mode;
}

void Settings::load(){
	size_t len = sizeof(SettingsStruct);
	esp_err_t err = nvs_get_blob(handle, BlobName, &settingsStruct, &len);
	if(err != ESP_OK){
		CMF_LOG(Settings, LogLevel::Warning, "No stored settings found, using defaults: %s", esp_err_to_name(err));
		settingsStruct = SettingsStruct();
	}

	uint8_t sensorVal = 0;
	if(nvs_get_u8(handle, SensorKey, &sensorVal) == ESP_OK && sensorVal <= static_cast<uint8_t>(SensorMode::AllOff)){
		sensorMode = static_cast<SensorMode>(sensorVal);
	}else{
		sensorMode = SensorMode::AllOn;
	}

	switch(settingsStruct.inactivityTimeout){
		case InactivityTimeout::Min2:
		case InactivityTimeout::Min5:
		case InactivityTimeout::Min10:
		case InactivityTimeout::Min30:
		case InactivityTimeout::Off:
			break;
		default:
			settingsStruct.inactivityTimeout = SettingsStruct().inactivityTimeout;
			break;
	}
}
