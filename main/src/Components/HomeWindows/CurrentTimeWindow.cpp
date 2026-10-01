#include "CurrentTimeWindow.h"

#include <LV_Interface/LVGL.h>
#include "Fonts/font.hpp"

static constexpr const char* TAG = "CurrentTime";

static const std::unordered_map<CurrentTimeData::Status, const char*> CurrentTimeSentences = {
	{ CurrentTimeData::Status::NoTime, "Time is fake" },
	{ CurrentTimeData::Status::ShowTime, "Present moment report!" },
};

CurrentTimeWindow::CurrentTimeWindow(lv_obj_t* parent, const CurrentTimeData* params) : HomeWindow(parent, TITLE){
	buildUI(*params);
}

void CurrentTimeWindow::onData(const BBData* data){
	const auto newData = *(const CurrentTimeData*)data;

	// Clear and rebuild UI
	lv_obj_clean(innerContent);
	buildUI(newData);
}

void CurrentTimeWindow::buildUI(const CurrentTimeData data){
	static constexpr uint8_t winW = 120;
	switch(data.status){
		case CurrentTimeData::Status::NoTime: {
			lv_obj_t* img = lv_image_create(innerContent);
			lv_image_set_src(img, theme->getAsset(Asset::TimeNo));
			lv_obj_move_to_index(img, 0);
			lv_obj_set_style_pad_ver(img, 6, 0);

			lv_obj_set_size(innerContent, 64, LV_SIZE_CONTENT);
			break;
		}
		case CurrentTimeData::Status::ShowTime: {
			lv_obj_t* timeContainer = lv_obj_create(innerContent);
			lv_obj_set_style_bg_opa(timeContainer, LV_OPA_TRANSP, 0);
			lv_obj_set_size(timeContainer, 86, 60);
			lv_obj_set_layout(timeContainer, LV_LAYOUT_FLEX);
			lv_obj_set_flex_flow(timeContainer, LV_FLEX_FLOW_COLUMN);
			lv_obj_set_flex_align(timeContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
			lv_obj_remove_flag(timeContainer, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_set_style_pad_left(timeContainer, 2, 0);

			// Time label format
			lv_obj_t* timeLabel = lv_label_create(timeContainer);
			lv_obj_add_style(timeLabel, labelDefaultStyle, 0);
			lv_obj_set_style_text_font(timeLabel, &lv_font_butter_clock, 0);
			lv_obj_set_style_text_letter_space(timeLabel, 2, 0);
			lv_obj_set_style_text_align(timeLabel, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_style_pad_ver(timeLabel, 16, 0);

			const std::string timeFormatStr = std::format("{:02}:{:02}", data.currentTime.tm_hour, data.currentTime.tm_min);
			lv_label_set_text(timeLabel, timeFormatStr.c_str());
			lv_obj_set_size(timeContainer, winW, LV_SIZE_CONTENT);
			lv_obj_set_size(timeLabel, winW, LV_SIZE_CONTENT);

			lv_obj_set_size(innerContent, winW, LV_SIZE_CONTENT);
			break;
		}
		default:
			ESP_LOGE(TAG, "CurrentTime status not recognised");
			break;
	}

	// Sentence label
	lv_obj_t* sentenceLabel = lv_label_create(innerContent);
	lv_label_set_text(sentenceLabel, CurrentTimeSentences.at(data.status));
	lv_label_set_long_mode(sentenceLabel, LV_LABEL_LONG_WRAP);
	lv_obj_add_style(sentenceLabel, labelDefaultStyle, 0);
	lv_obj_set_size(sentenceLabel, winW, LV_SIZE_CONTENT);
	lv_obj_set_style_text_align(sentenceLabel, LV_TEXT_ALIGN_CENTER, 0);

	updateLayout();
}
