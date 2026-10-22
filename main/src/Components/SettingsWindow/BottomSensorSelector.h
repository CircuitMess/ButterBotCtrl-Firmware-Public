#ifndef BUTTERBOTCTRL_FIRMWARE_BOTTOMSENSORSELECTOR_H
#define BUTTERBOTCTRL_FIRMWARE_BOTTOMSENSORSELECTOR_H

#include <array>
#include <functional>
#include <CtrlData.h>
#include <LV_Interface/LVObject.h>
#include <LV_Interface/LVStyle.h>
#include <Services/ThemeService.h>

class BottomSensorSelector : public LVObject {
public:
	BottomSensorSelector(lv_obj_t* parent, BottomSensorMode initVal, const std::function<void(uint32_t)>& keyCb, const std::function<void(BottomSensorMode)>& valCb);
	~BottomSensorSelector() override;

	void setValue(BottomSensorMode value);

	lv_obj_t* widgetLabel;

private:
	ThemeService* theme;
	LVStyle labelDefaultStyle;

	lv_obj_t* selectorLabel;

	uint8_t currentIndex;
	static constexpr uint8_t OptionsNum = 3;
	static constexpr std::array<BottomSensorMode, OptionsNum> OptionValueMap = {
		BottomSensorMode::Off,
		BottomSensorMode::Reduced,
		BottomSensorMode::On
	};
	static constexpr std::array<const char*, OptionsNum> OptionNameMap = {
		"OFF",
		"REDUCED",
		"ON"
	};

	std::function<void(uint32_t)> keyCb;
	const std::function<void(BottomSensorMode)> valCb;

	void buildUI(lv_obj_t* parent);
};


#endif //BUTTERBOTCTRL_FIRMWARE_BOTTOMSENSORSELECTOR_H