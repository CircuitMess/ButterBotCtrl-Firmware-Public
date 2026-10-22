#ifndef BUTTERBOTCTRL_FIRMWARE_FRONTSENSORSELECTOR_H
#define BUTTERBOTCTRL_FIRMWARE_FRONTSENSORSELECTOR_H

#include <array>
#include <functional>
#include <CtrlData.h>
#include <LV_Interface/LVObject.h>
#include <LV_Interface/LVStyle.h>
#include <Services/ThemeService.h>

class FrontSensorSelector : public LVObject {
public:
	FrontSensorSelector(lv_obj_t* parent, FrontSensorMode initVal, const std::function<void(uint32_t)>& keyCb, const std::function<void(FrontSensorMode)>& valCb);
	~FrontSensorSelector() override;

	void setValue(FrontSensorMode value);

	lv_obj_t* widgetLabel;

private:
	ThemeService* theme;
	LVStyle labelDefaultStyle;

	lv_obj_t* selectorLabel;

	uint8_t currentIndex;
	static constexpr uint8_t OptionsNum = 2;
	static constexpr std::array<FrontSensorMode, OptionsNum> OptionValueMap = {
		FrontSensorMode::Off,
		FrontSensorMode::On
	};
	static constexpr std::array<const char*, OptionsNum> OptionNameMap = {
		"OFF",
		"ON"
	};

	std::function<void(uint32_t)> keyCb;
	const std::function<void(FrontSensorMode)> valCb;

	void buildUI(lv_obj_t* parent);
};


#endif //BUTTERBOTCTRL_FIRMWARE_FRONTSENSORSELECTOR_H