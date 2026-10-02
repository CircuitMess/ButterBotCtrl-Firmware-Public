#include "GuideElement.h"

#include "Fonts/font.hpp"
#include <vector>
#include <Services/Settings.h>

static constexpr const char* Lines[] = {
		"Override: Hold for RC",
		"Poke: Existential dread",
		"Shut up: (Un)Mute",
		"Summon: Press to listen, hold to summon",
		"Joystick press: Actions",
};

GuideElement::GuideElement(lv_obj_t* parent) : LVObject(parent){
	theme = Application::getApp()->getService<ThemeService>();
	buildUI();
}

void GuideElement::buildUI(){
	// Kept out of the parent's flex flow and scroll (same pattern as HomeWindow's corner)
	lv_obj_add_flag(*this, LV_OBJ_FLAG_IGNORE_LAYOUT);
	lv_obj_add_flag(*this, LV_OBJ_FLAG_FLOATING);
	lv_obj_remove_flag(*this, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_set_size(*this, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_layout(*this, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_row(*this, 8, 0);
	lv_obj_set_style_bg_opa(*this, LV_OPA_TRANSP, 0);

	const lv_color_t colorPrim = theme->getPrimaryColor();

	// reminder while robot proximity sensors are ignored
	std::vector<const char*> lines(std::begin(Lines), std::end(Lines));
	if(Settings* settings = Application::getApp()->getService<Settings>()){
		switch(settings->getSensorMode()){
			case SensorMode::FrontOff: lines.push_back("SENSORS: FRONT OFF"); break;
			case SensorMode::FloorOff: lines.push_back("SENSORS: FLOOR OFF"); break;
			case SensorMode::AllOff: lines.push_back("SENSORS: ALL OFF"); break;
			default: break;
		}
	}

	for(const char* text : lines){
		lv_obj_t* label = lv_label_create(*this);
		lv_label_set_text_static(label, text);
		lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
		lv_obj_set_width(label, lv_pct(100));
		lv_obj_set_style_text_font(label, &lv_font_butter, 0);
		lv_obj_set_style_text_color(label, colorPrim, 0);
		lv_obj_set_style_text_line_space(label, 3, 0);
		lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	}
}
