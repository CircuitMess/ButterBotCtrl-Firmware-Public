#ifndef BB_CTRL_CONTROLLER_JOYSTICKINPUTLVGL_H
#define BB_CTRL_CONTROLLER_JOYSTICKINPUTLVGL_H

#include <atomic>
#include <lvgl.h>
#include <LV_Interface/InputLVGL.h>
#include <Misc/Enum.h>
#include <Services/ButtonInput.h>

#include "../Periph/Joystick.h"

class JoystickInputLVGL : public InputLVGL {
	GENERATED_BODY(JoystickInputLVGL, InputLVGL, CONSTRUCTOR_PACK(Joystick*, ButtonInput*, Enum<int>))

public:
	JoystickInputLVGL(Joystick* joystick = nullptr, ButtonInput* bi = nullptr, Enum<int> enterButton = 0);

	void postInitProperties() noexcept override;

private:
	static constexpr int DirThreshold = 40;

	Joystick* joystick;
	ButtonInput* bi;
	Enum<int> enterButton;

	std::atomic<bool> enterPressed{ false };
	lv_key_t lastReportedKey = static_cast<lv_key_t>(0);
	bool lastReportedPressed = false;

	void readJoystick(lv_indev_data_t* data);
	void onButtonInput(Enum<int> btn, ButtonInput::Action action);
};

#endif //BB_CTRL_CONTROLLER_JOYSTICKINPUTLVGL_H
