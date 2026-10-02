#ifndef BUTTERBOTCTRL_FIRMWARE_ROAMINGSELECTOR_H
#define BUTTERBOTCTRL_FIRMWARE_ROAMINGSELECTOR_H

#include <LV_Interface/LVObject.h>
#include <LV_Interface/LVStyle.h>
#include <Services/ThemeService.h>
#include <Services/Settings.h>

// Settings row ROAMING - OFF keeps the robot in place while idle (no wandering, no turning and driving to faces)
class RoamingSelector : public LVObject {
public:
	RoamingSelector(lv_obj_t* parent, bool initVal, const std::function<void(uint32_t)>& keyCb, const std::function<void(bool)>& valCb);
	~RoamingSelector() override;

	lv_obj_t* widgetLabel;

private:
	ThemeService* theme;
	LVStyle labelDefaultStyle;

	lv_obj_t* selectorLabel;

	uint8_t currentIndex;
	static constexpr uint8_t OptionsNum = 2;
	static constexpr std::array<bool, OptionsNum> OptionValueMap = { true, false };
	static constexpr std::array<const char*, OptionsNum> OptionNameMap = { "ON", "OFF" };

	std::function<void(uint32_t)> keyCb;
	const std::function<void(bool)> valCb;

	void buildUI(lv_obj_t* parent);
};


#endif //BUTTERBOTCTRL_FIRMWARE_ROAMINGSELECTOR_H
