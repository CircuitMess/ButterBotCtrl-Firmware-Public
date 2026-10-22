#include "BottomSensorSelector.h"

#include <Fonts/font.hpp>
#include <misc/lv_event_private.h>

BottomSensorSelector::BottomSensorSelector(lv_obj_t* parent, const BottomSensorMode initVal, const std::function<void(uint32_t)>& keyCb, const std::function<void(BottomSensorMode)>& valCb)
	: LVObject(parent), keyCb(keyCb), valCb(valCb){
	const Application* app = Application::getApp();
	theme = app->getService<ThemeService>();

	currentIndex = 0;
	for(uint8_t i = 0; i < OptionsNum; i++){
		if(OptionValueMap.at(i) == initVal){
			currentIndex = i;
			break;
		}
	}

	lv_obj_add_event_cb(*this, [](lv_event_t* e) {
		const uint32_t key = lv_event_get_key(e);
		BottomSensorSelector* bottomSensorSelector = (BottomSensorSelector*)(e->user_data);

		if(key != LV_KEY_RIGHT && key != LV_KEY_LEFT && key != LV_KEY_UP && key != LV_KEY_DOWN){
			return;
		}
		if(key == LV_KEY_UP || key == LV_KEY_DOWN){
			bottomSensorSelector->keyCb(key);
			return;
		}
		if(key == LV_KEY_RIGHT){
			if(bottomSensorSelector->currentIndex >= OptionsNum - 1){
				bottomSensorSelector->currentIndex = 0;
			} else{
				bottomSensorSelector->currentIndex += 1;
			}
		} else{
			if(bottomSensorSelector->currentIndex <= 0){
				bottomSensorSelector->currentIndex = OptionsNum - 1;
			} else{
				bottomSensorSelector->currentIndex -= 1;
			}
		}

		lv_label_set_text(bottomSensorSelector->selectorLabel, OptionNameMap.at(bottomSensorSelector->currentIndex));
		bottomSensorSelector->valCb(OptionValueMap.at(bottomSensorSelector->currentIndex));
	}, LV_EVENT_KEY, this);

	buildUI(parent);
}

BottomSensorSelector::~BottomSensorSelector(){}

void BottomSensorSelector::setValue(const BottomSensorMode value){
	for(uint8_t i = 0; i < OptionsNum; i++){
		if(OptionValueMap.at(i) == value){
			currentIndex = i;
			lv_label_set_text(selectorLabel, OptionNameMap.at(currentIndex));
			return;
		}
	}
}

void BottomSensorSelector::buildUI(lv_obj_t* parent){
	const lv_color_t colorPrim = theme->getPrimaryColor();
	const lv_color_t colorTert = theme->getTertiaryColor();

	lv_obj_set_size(*this, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_bg_opa(*this, LV_OPA_TRANSP, 0);

	lv_style_set_text_font(labelDefaultStyle, &lv_font_butter);
	lv_style_set_text_line_space(labelDefaultStyle, 3);
	lv_style_set_text_color(labelDefaultStyle, colorPrim);
	lv_style_set_size(labelDefaultStyle, 80, LV_SIZE_CONTENT);
	lv_style_set_bg_opa(labelDefaultStyle, LV_OPA_TRANSP);

	widgetLabel = lv_label_create(*this);
	lv_obj_add_style(widgetLabel, labelDefaultStyle, 0);
	lv_label_set_text(widgetLabel, "BOTTOM SENSOR");
	lv_obj_set_width(widgetLabel, 62);
	lv_obj_set_pos(widgetLabel, 0, 5);

	lv_obj_t* selector = lv_obj_create(*this);
	lv_obj_set_size(selector, 51, 18);
	lv_obj_set_pos(selector, 62, 0);
	lv_obj_set_style_border_width(selector, 1, 0);
	lv_obj_set_style_border_color(selector, colorPrim, 0);
	lv_obj_set_style_bg_color(selector, colorTert, 0);
	lv_obj_set_style_bg_opa(selector, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_image_src(selector, theme->getAsset(Asset::Grid), 0);

	selectorLabel = lv_label_create(selector);
	lv_obj_add_style(selectorLabel, labelDefaultStyle, 0);
	lv_label_set_text(selectorLabel, OptionNameMap.at(currentIndex));
	lv_obj_set_width(selectorLabel, 37);
	lv_obj_set_pos(selectorLabel, 7, 4);
	lv_obj_set_style_text_align(selectorLabel, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_bg_opa(selectorLabel, LV_OPA_TRANSP, 0);

	lv_obj_t* selectorLeft = lv_image_create(selector);
	lv_image_set_src(selectorLeft, theme->getAsset(Asset::SettingsLeft));
	lv_obj_set_style_bg_opa(selectorLeft, LV_OPA_TRANSP, 0);
	lv_obj_set_pos(selectorLeft, 2, 3);

	lv_obj_t* selectorRight = lv_image_create(selector);
	lv_image_set_src(selectorRight, theme->getAsset(Asset::SettingsRight));
	lv_obj_set_style_bg_opa(selectorRight, LV_OPA_TRANSP, 0);
	lv_obj_set_pos(selectorRight, 43, 3);
}