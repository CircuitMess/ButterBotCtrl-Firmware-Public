#include "SensorSelector.h"

#include <Fonts/font.hpp>
#include <misc/lv_event_private.h>

SensorSelector::SensorSelector(lv_obj_t* parent, const SensorMode initVal, const std::function<void(uint32_t)>& keyCb, const std::function<void(SensorMode)>& valCb)
	: LVObject(parent), keyCb(keyCb), valCb(valCb){
	const auto app = Application::getApp();
	theme = app->getService<ThemeService>();

	currentIndex = 0;
	for(uint8_t i = 0; i < OptionsNum; i++){
		if(OptionValueMap.at(i) == initVal){
			currentIndex = i;
			break;
		}
	}

	// CALLBACKS
	lv_obj_add_event_cb(*this, [](lv_event_t* e) {
		const uint32_t key = lv_event_get_key(e);
		const auto sensorSelector = (SensorSelector*)(e->user_data);

		if(key != LV_KEY_RIGHT && key != LV_KEY_LEFT && key != LV_KEY_UP && key != LV_KEY_DOWN) return;
		if(key == LV_KEY_UP || key == LV_KEY_DOWN){
			// WINDOW CALLBACK
			sensorSelector->keyCb(key);
			return;
		}
		if(key == LV_KEY_RIGHT){
			if(sensorSelector->currentIndex >= OptionsNum - 1){
				sensorSelector->currentIndex = 0;
			} else{
				sensorSelector->currentIndex += 1;
			}
		} else{
			if(sensorSelector->currentIndex <= 0){
				sensorSelector->currentIndex = OptionsNum - 1;
			} else{
				sensorSelector->currentIndex -= 1;
			}
		}

		lv_label_set_text(sensorSelector->selectorLabel, OptionNameMap.at(sensorSelector->currentIndex));
		sensorSelector->valCb(OptionValueMap.at(sensorSelector->currentIndex));
	}, LV_EVENT_KEY, this);

	buildUI(parent);
}

SensorSelector::~SensorSelector(){}

void SensorSelector::buildUI(lv_obj_t* parent){
	const lv_color_t colorPrim = theme->getPrimaryColor();
	const lv_color_t colorTert = theme->getTertiaryColor();

	lv_obj_set_size(*this, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_bg_opa(*this, LV_OPA_TRANSP, 0);

	// LABEL DEFAULT STYLE
	lv_style_set_text_font(labelDefaultStyle, &lv_font_butter);
	lv_style_set_text_line_space(labelDefaultStyle, 3);
	lv_style_set_text_color(labelDefaultStyle, colorPrim);
	lv_style_set_size(labelDefaultStyle, 80, LV_SIZE_CONTENT);
	lv_style_set_bg_opa(labelDefaultStyle, LV_OPA_TRANSP);

	// Sensor widget label
	widgetLabel = lv_label_create(*this);
	lv_obj_add_style(widgetLabel, labelDefaultStyle, 0);
	lv_label_set_text(widgetLabel, "SENSOR");
	lv_obj_set_width(widgetLabel, 32);
	lv_obj_set_pos(widgetLabel, 2, 5);

	// Selector container
	lv_obj_t* selector = lv_obj_create(*this);
	lv_obj_set_size(selector, 77, 18);
	lv_obj_set_pos(selector, 34, 0);
	lv_obj_set_style_border_width(selector, 1, 0);
	lv_obj_set_style_border_color(selector, colorPrim, 0);
	lv_obj_set_style_bg_color(selector, colorTert, 0);
	lv_obj_set_style_bg_opa(selector, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_image_src(selector, theme->getAsset(Asset::Grid), 0);

	// Selector label
	selectorLabel = lv_label_create(selector);
	lv_obj_add_style(selectorLabel, labelDefaultStyle, 0);
	lv_label_set_text(selectorLabel, OptionNameMap.at(currentIndex));
	lv_obj_set_width(selectorLabel, 63);
	lv_obj_set_pos(selectorLabel, 7, 4);
	lv_obj_set_style_text_align(selectorLabel, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_bg_opa(selectorLabel, LV_OPA_TRANSP, 0);

	// Selector left icon
	lv_obj_t* selectorLeft = lv_image_create(selector);
	lv_image_set_src(selectorLeft, theme->getAsset(Asset::SettingsLeft));
	lv_obj_set_style_bg_opa(selectorLeft, LV_OPA_TRANSP, 0);
	lv_obj_set_pos(selectorLeft, 2, 3);

	// Selector right icon
	lv_obj_t* selectorRight = lv_image_create(selector);
	lv_image_set_src(selectorRight, theme->getAsset(Asset::SettingsRight));
	lv_obj_set_style_bg_opa(selectorRight, LV_OPA_TRANSP, 0);
	lv_obj_set_pos(selectorRight, 67, 3);
}
