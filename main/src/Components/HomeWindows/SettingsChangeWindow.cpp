#include "SettingsChangeWindow.h"

#include <Phrases.h>

SettingsChangeWindow::SettingsChangeWindow(lv_obj_t* parent, const SettingsChangeData* params) : HomeWindow(parent, TITLE){
	sentenceLabel = lv_label_create(innerContent);

	buildUI(*params);
}

void SettingsChangeWindow::onData(const BBData* data){
	const SettingsChangeData* change = static_cast<const SettingsChangeData*>(data);
	lv_label_set_text(sentenceLabel, Phrases::mapShown(change->phrase, change->id).c_str());
	updateLayout();
}

void SettingsChangeWindow::buildUI(const SettingsChangeData data){
	lv_label_set_text(sentenceLabel, Phrases::mapShown(data.phrase, data.id).c_str());

	lv_label_set_long_mode(sentenceLabel, LV_LABEL_LONG_WRAP);
	lv_obj_add_style(sentenceLabel, labelDefaultStyle, 0);

	updateLayout();
}