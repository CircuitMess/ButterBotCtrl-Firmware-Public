#include "SettingsWindow.h"

#include "Fonts/font.hpp"
#include "Services/Com.h"

SettingsWindow::SettingsWindow(lv_obj_t* parent, lv_group_t* inputGroup, const std::function<void(const Theme& newTheme)>& themeCb)
	: LVObject(parent), inputGroup(inputGroup), themeCb(themeCb){
	const auto app = Application::getApp();
	theme = app->getService<ThemeService>();
	settings = app->getService<Settings>();
	ledService = app->getService<LED<LEDs, RGB_LEDs>>();

	buildUI(parent);
}

SettingsWindow::~SettingsWindow(){
	lv_anim_delete(themeSelector, nullptr);
	lv_anim_delete(sleepSelector, nullptr);
	lv_anim_delete(brightnessSlider, nullptr);
	lv_anim_delete(sensorSelector, nullptr);
	lv_anim_delete(roamingSelector, nullptr);
}

void SettingsWindow::buildUI(lv_obj_t* parent){
	const lv_color_t colorPrim = theme->getPrimaryColor();
	const lv_color_t colorTert = theme->getTertiaryColor();

	SettingsStruct initSet = settings->get();

	titleEl = lv_obj_create(*this);
	titleLabel = lv_label_create(titleEl);
	lv_label_set_text(titleLabel, WindowTitle);

	corner = lv_image_create(*this);
	lv_obj_add_flag(corner, LV_OBJ_FLAG_IGNORE_LAYOUT);
	lv_obj_add_flag(corner, LV_OBJ_FLAG_FLOATING);
	innerContent = lv_obj_create(*this);

	// CONTAINER
	lv_obj_set_size(*this, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_layout(*this, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_remove_flag(*this, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_margin_top(*this, 2, 0);

	lv_obj_set_style_border_width(innerContent, 1, 0);
	lv_obj_set_style_border_color(innerContent, colorPrim, 0);
	lv_obj_set_style_bg_color(*this, colorTert, 0);
	lv_obj_set_style_bg_image_src(*this, theme->getAsset(Asset::Grid), 0);

	// INNER WINDOW
	lv_obj_set_size(innerContent, WindowWidth, WindowHeight);
	lv_obj_set_style_bg_color(innerContent, colorTert, 0);
	lv_obj_set_style_bg_opa(innerContent, LV_OPA_COVER, 0);
	lv_obj_remove_flag(innerContent, LV_OBJ_FLAG_SCROLLABLE);

	// TITLE
	lv_obj_set_size(titleEl, LV_SIZE_CONTENT, 6);
	lv_obj_set_layout(titleEl, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(titleEl, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(titleEl, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

	lv_obj_set_style_pad_hor(titleEl, 1, 0);
	lv_obj_set_style_text_color(titleEl, colorTert, 0);
	lv_obj_set_style_text_font(titleEl, &lv_font_butter, 0);
	lv_obj_set_style_bg_color(titleEl, colorPrim, 0);
	lv_obj_set_style_bg_opa(titleEl, LV_OPA_COVER, 0);

	lv_obj_set_style_pad_right(titleLabel, 6, 0);
	lv_obj_set_style_pad_left(titleLabel, 1, 0);

	// LABEL DEFAULT STYLE
	lv_style_set_text_font(labelDefaultStyle, &lv_font_butter);
	lv_style_set_text_line_space(labelDefaultStyle, 3);
	lv_style_set_text_color(labelDefaultStyle, colorPrim);
	lv_style_set_size(labelDefaultStyle, 80, LV_SIZE_CONTENT);
	lv_style_set_pad_all(labelDefaultStyle, 2);
	lv_style_set_bg_opa(labelDefaultStyle, LV_OPA_TRANSP);

	// CORNER
	lv_obj_set_size(corner, 6, 6);
	lv_obj_set_style_bg_color(corner, colorTert, 0);
	lv_obj_set_style_bg_opa(corner, LV_OPA_COVER, 0);
	lv_image_set_src(corner, theme->getAsset(Asset::Corner));

	/************************ CONTENT ******************************/
	// Blink animation
	lv_anim_init(&blinkAnim);
	lv_anim_set_duration(&blinkAnim, 300);
	lv_anim_set_playback_duration(&blinkAnim, 300);
	lv_anim_set_repeat_count(&blinkAnim, LV_ANIM_REPEAT_INFINITE);
	lv_anim_set_path_cb(&blinkAnim, lv_anim_path_ease_in_out);
	lv_anim_set_values(&blinkAnim, LV_OPA_COVER, LV_OPA_0);
	lv_anim_set_exec_cb(&blinkAnim, [](void* var, const int32_t v) {
		lv_obj_set_style_opa((lv_obj_t*)var, v, 0);
	});

	// Widget switch callback
	auto switchCb = [this](const uint32_t key) {
		lv_anim_delete(themeSelector->widgetLabel, nullptr);
		lv_anim_delete(sleepSelector->widgetLabel, nullptr);
		lv_anim_delete(brightnessSlider->widgetLabel, nullptr);
		lv_anim_delete(sensorSelector->widgetLabel, nullptr);
		lv_anim_delete(roamingSelector->widgetLabel, nullptr);
		lv_obj_set_style_opa(themeSelector->widgetLabel, LV_OPA_COVER, 0);
		lv_obj_set_style_opa(sleepSelector->widgetLabel, LV_OPA_COVER, 0);
		lv_obj_set_style_opa(brightnessSlider->widgetLabel, LV_OPA_COVER, 0);
		lv_obj_set_style_opa(sensorSelector->widgetLabel, LV_OPA_COVER, 0);
		lv_obj_set_style_opa(roamingSelector->widgetLabel, LV_OPA_COVER, 0);

		if(key == LV_KEY_DOWN){
			currentFocusIndex = (currentFocusIndex + 1) % RowCount;
		} else{
			currentFocusIndex = (currentFocusIndex + RowCount - 1) % RowCount;
		}

		if(currentFocusIndex == 0){
			lv_anim_set_var(&blinkAnim, themeSelector->widgetLabel);
		} else if(currentFocusIndex == 1){
			lv_anim_set_var(&blinkAnim, sleepSelector->widgetLabel);
		} else if(currentFocusIndex == 2){
			lv_anim_set_var(&blinkAnim, brightnessSlider->widgetLabel);
		} else if(currentFocusIndex == 3){
			lv_anim_set_var(&blinkAnim, sensorSelector->widgetLabel);
		} else{
			lv_anim_set_var(&blinkAnim, roamingSelector->widgetLabel);
		}
		lv_anim_start(&blinkAnim);
		lv_group_focus_obj(lv_obj_get_child(innerContent, currentFocusIndex));
	};

	// Theme selector
	themeSelector = new ThemeSelector(innerContent, switchCb, themeCb);
	lv_obj_set_pos(*themeSelector, 2, 1);
	lv_group_add_obj(inputGroup, *themeSelector);
	// Set init animation
	lv_anim_set_var(&blinkAnim, themeSelector->widgetLabel);
	lv_anim_start(&blinkAnim);

	// Sleep timeout selector
	auto sleepValCb = [this](const InactivityTimeout timeout) {
		SettingsStruct currentSet = settings->get();
		currentSet.inactivityTimeout = timeout;
		settings->set(currentSet);
	};
	sleepSelector = new SleepSelector(innerContent, initSet.inactivityTimeout, switchCb, sleepValCb);
	lv_obj_set_pos(*sleepSelector, 2, 17);
	lv_group_add_obj(inputGroup, *sleepSelector);

	// Brightness slider
	auto brightnessValCb = [this](const float value) {
		ledService->on(LEDs::TftBacklight, value);
		SettingsStruct currentSet = settings->get();
		currentSet.screenBrightness = value;
		settings->set(currentSet);
	};
	brightnessSlider = new BrightnessSlider(innerContent, initSet.screenBrightness, brightnessValCb, switchCb);
	lv_obj_set_pos(*brightnessSlider, 2, 34);
	lv_group_add_obj(inputGroup, *brightnessSlider);

	// proximity sensor filter, sent to the robot right away and on every connect
	auto sensorValCb = [this](const SensorMode mode) {
		settings->setSensorMode(mode);
		if(Com* com = Application::getApp()->getService<Com>()){
			com->setSensorCommand(sensorModeToCommand(mode));
		}
	};
	sensorSelector = new SensorSelector(innerContent, settings->getSensorMode(), switchCb, sensorValCb);
	lv_obj_set_pos(*sensorSelector, 2, 42);
	lv_group_add_obj(inputGroup, *sensorSelector);

	// idle roaming on / off, sent to the robot right away and on every connect
	auto roamingValCb = [this](const bool roaming) {
		settings->setRoaming(roaming);
		if(Com* com = Application::getApp()->getService<Com>()){
			com->setRoamingCommand(roaming ? Ctrl::RoamingOn : Ctrl::RoamingOff);
		}
	};
	roamingSelector = new RoamingSelector(innerContent, settings->getRoaming(), switchCb, roamingValCb);
	lv_obj_set_pos(*roamingSelector, 2, 58);
	lv_group_add_obj(inputGroup, *roamingSelector);

	lv_group_set_editing(inputGroup, true);

	updateLayout();
}

void SettingsWindow::updateLayout(){
	lv_obj_set_size(titleEl, WindowWidth, 6);
	lv_obj_set_width(*this, WindowWidth);
	lv_obj_align_to(corner, *this, LV_ALIGN_TOP_RIGHT, 0, 0);
};
