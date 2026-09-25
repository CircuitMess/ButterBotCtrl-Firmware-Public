#ifndef BUTTERBOTCTRL_FIRMWARE_SENSORSELECTOR_H
#define BUTTERBOTCTRL_FIRMWARE_SENSORSELECTOR_H

#include <LV_Interface/LVObject.h>
#include <LV_Interface/LVStyle.h>
#include <Services/ThemeService.h>
#include <Services/Settings.h>

// Settings row that turns the robot's proximity sensors off
class SensorSelector : public LVObject {
public:
	SensorSelector(lv_obj_t* parent, SensorMode initVal, const std::function<void(uint32_t)>& keyCb, const std::function<void(SensorMode)>& valCb);
	~SensorSelector() override;

	lv_obj_t* widgetLabel;

private:
	ThemeService* theme;
	LVStyle labelDefaultStyle;

	lv_obj_t* selectorLabel;

	uint8_t currentIndex;
	static constexpr uint8_t OptionsNum = 4;
	static constexpr std::array<SensorMode, OptionsNum> OptionValueMap = {
		SensorMode::AllOn,
		SensorMode::FrontOff,
		SensorMode::FloorOff,
		SensorMode::AllOff
	};
	static constexpr std::array<const char*, OptionsNum> OptionNameMap = {
		"ALL ON",
		"FRONT OFF",
		"FLOOR OFF",
		"ALL OFF"
	};

	std::function<void(uint32_t)> keyCb;
	const std::function<void(SensorMode)> valCb;

	void buildUI(lv_obj_t* parent);
};


#endif //BUTTERBOTCTRL_FIRMWARE_SENSORSELECTOR_H
