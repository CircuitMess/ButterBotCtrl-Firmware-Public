#include "SettingsWindow.h"

#include "Fonts/font.hpp"

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
	lv_anim_delete(frontSensorSelector, nullptr);
	lv_anim_delete(bottomSensorSelector, nullptr);
	lv_anim_delete(activitySelector, nullptr);
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
	lv_obj_set_scroll_dir(innerContent, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(innerContent, LV_SCROLLBAR_MODE_OFF);

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
		lv_anim_delete(frontSensorSelector->widgetLabel, nullptr);
		lv_anim_delete(bottomSensorSelector->widgetLabel, nullptr);
		lv_anim_delete(activitySelector->widgetLabel, nullptr);
		lv_obj_set_style_opa(themeSelector->widgetLabel, LV_OPA_COVER, 0);
		lv_obj_set_style_opa(sleepSelector->widgetLabel, LV_OPA_COVER, 0);
		lv_obj_set_style_opa(brightnessSlider->widgetLabel, LV_OPA_COVER, 0);
		lv_obj_set_style_opa(frontSensorSelector->widgetLabel, LV_OPA_COVER, 0);
		lv_obj_set_style_opa(bottomSensorSelector->widgetLabel, LV_OPA_COVER, 0);
		lv_obj_set_style_opa(activitySelector->widgetLabel, LV_OPA_COVER, 0);

		if(key == LV_KEY_DOWN){
			currentFocusIndex = (currentFocusIndex + 1) % WidgetsNum;
		} else{
			currentFocusIndex = (currentFocusIndex + WidgetsNum - 1) % WidgetsNum;
		}

		if(currentFocusIndex == 0){
			lv_anim_set_var(&blinkAnim, themeSelector->widgetLabel);
		} else if(currentFocusIndex == 1){
			lv_anim_set_var(&blinkAnim, sleepSelector->widgetLabel);
		} else if(currentFocusIndex == 2){
			lv_anim_set_var(&blinkAnim, brightnessSlider->widgetLabel);
		} else if(currentFocusIndex == 3){
			lv_anim_set_var(&blinkAnim, frontSensorSelector->widgetLabel);
		} else if(currentFocusIndex == 4){
			lv_anim_set_var(&blinkAnim, bottomSensorSelector->widgetLabel);
		} else{
			lv_anim_set_var(&blinkAnim, activitySelector->widgetLabel);
		}
		lv_anim_start(&blinkAnim);

		lv_obj_t* focused = lv_obj_get_child(innerContent, currentFocusIndex);
		lv_group_focus_obj(focused);

		lv_obj_update_layout(focused);
		const int32_t rowTop = lv_obj_get_style_y(focused, 0) - ScrollMargin;
		const int32_t rowBottom = lv_obj_get_style_y(focused, 0) + lv_obj_get_height(focused) + ScrollMargin;
		const int32_t viewHeight = lv_obj_get_content_height(innerContent);
		int32_t scrollY = lv_obj_get_scroll_y(innerContent);
		if(rowTop < scrollY){
			scrollY = rowTop;
		}
		if(rowBottom > scrollY + viewHeight){
			scrollY = rowBottom - viewHeight;
		}
		if(scrollY < 0){
			scrollY = 0;
		}
		lv_obj_scroll_to_y(innerContent, scrollY, LV_ANIM_OFF);
	};

	// Theme selector
	themeSelector = new ThemeSelector(innerContent, switchCb, themeCb);
	lv_obj_set_pos(*themeSelector, 2, 4);
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
	lv_obj_set_pos(*sleepSelector, 2, 26);
	lv_group_add_obj(inputGroup, *sleepSelector);

	// Brightness slider
	auto brightnessValCb = [this](const float value) {
		ledService->on(LEDs::TftBacklight, value);
		SettingsStruct currentSet = settings->get();
		currentSet.screenBrightness = value;
		settings->set(currentSet);
	};
	brightnessSlider = new BrightnessSlider(innerContent, initSet.screenBrightness, brightnessValCb, switchCb);
	lv_obj_set_pos(*brightnessSlider, 2, 48);
	lv_group_add_obj(inputGroup, *brightnessSlider);

	const std::function<void(FrontSensorMode)> frontSensorValCb = [this](const FrontSensorMode mode) {
		SettingsStruct currentSet = settings->get();
		currentSet.frontSensor = mode;
		settings->set(currentSet);
	};
	frontSensorSelector = new FrontSensorSelector(innerContent, initSet.frontSensor, switchCb, frontSensorValCb);
	lv_obj_set_pos(*frontSensorSelector, 2, 59);
	lv_group_add_obj(inputGroup, *frontSensorSelector);

	const std::function<void(BottomSensorMode)> bottomSensorValCb = [this](const BottomSensorMode mode) {
		SettingsStruct currentSet = settings->get();
		currentSet.bottomSensor = mode;
		settings->set(currentSet);
	};
	bottomSensorSelector = new BottomSensorSelector(innerContent, initSet.bottomSensor, switchCb, bottomSensorValCb);
	lv_obj_set_pos(*bottomSensorSelector, 2, 81);
	lv_group_add_obj(inputGroup, *bottomSensorSelector);

	const std::function<void(ActivityMode)> activityValCb = [this](const ActivityMode mode) {
		SettingsStruct currentSet = settings->get();
		currentSet.activityMode = mode;
		settings->set(currentSet);
	};
	activitySelector = new ActivitySelector(innerContent, initSet.activityMode, switchCb, activityValCb);
	lv_obj_set_pos(*activitySelector, 2, 103);
	lv_obj_set_style_margin_bottom(*activitySelector, 4, 0);
	lv_group_add_obj(inputGroup, *activitySelector);

	lv_group_set_editing(inputGroup, true);

	updateLayout();
}

void SettingsWindow::syncRobotSettings(const SettingsStruct& currentSet){
	frontSensorSelector->setValue(currentSet.frontSensor);
	bottomSensorSelector->setValue(currentSet.bottomSensor);
	activitySelector->setValue(currentSet.activityMode);
}

void SettingsWindow::updateLayout(){
	lv_obj_set_size(titleEl, WindowWidth, 6);
	lv_obj_set_width(*this, WindowWidth);
	lv_obj_align_to(corner, *this, LV_ALIGN_TOP_RIGHT, 0, 0);
};
