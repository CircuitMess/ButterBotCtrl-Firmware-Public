#include <nvs_flash.h>
#include <esp_random.h>
#include <cstdlib>
#include <Core/EntryPoint.h>
#include <Devices/Display.h>
#include <Drivers/Input/InputGPIO.h>
#include <Drivers/Output/OutputGPIO.h>
#include <FileSystem/SPIFFS.h>
#include <LV_Interface/FSLVGL.h>
#include <LV_Interface/LVGL.h>
#include <Periphery/WiFi.h>
#include <Services/ButtonInput.h>
#include <Services/LED/LED.h>
#include <Util/EfuseMeta.h>
#include <Util/WiFiAccessPoint.h>
#include <bootloader_random.h>

#include "Periph/Bluetooth.h"
#include "Periph/Joystick.h"
#include "BLE/GAP.h"
#include "BLE/Client.h"
#include "Services/Com.h"

#include "HardwareConfiguration.hpp"
#include "Enums.hpp"
#include "Services/Battery.h"
#include "Services/BatteryIndicator.h"
#include "Services/JoystickInputLVGL.h"
#include "Services/ShutdownService.h"
#include "Services/LEDController.h"
#include "Services/ThemeService.h"
#include "Services/RobotState.h"
#include "Services/Settings.h"
#include "Screens/IntroScreen.h"
#include "JigHWTest/JigHWTest.h"

class ButterBotController : public Application {
	GENERATED_BODY(ButterBotController, Application, void)

protected:
	virtual void begin() noexcept override{
		bootloader_random_enable();
		srand(esp_random());
		bootloader_random_disable();

		esp_log_level_set("*", ESP_LOG_WARN);

		// Stack overflow otherwise - increase Threaded/AsyncEntity stack size if logging is needed
		esp_log_level_set("Com", ESP_LOG_NONE);

		auto ret = nvs_flash_init();
		if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
			ESP_ERROR_CHECK(nvs_flash_erase());
			ret = nvs_flash_init();
		}
		ESP_ERROR_CHECK(ret);

		if(JigHWTest::checkJig()){
			ESP_LOGE("checkJig", "Jig\n");

			JigHWTest* test = new JigHWTest();
			test->start();
			vTaskDelete(nullptr);
		} else{
			ESP_LOGE("checkJig", "Hello\n");
		}

		if(!EfuseMeta::check()){
			while(true){
				vTaskDelay(1000);
				EfuseMeta::log();
			}
		}

		Battery* battery = registerService<Battery>();

		// Initial battery level check (for early shutdown)
		if(battery->isShutdown()){
			ESP_LOGE("init_bat_check", "CRITICAL");
			ESP_LOGE("init_bat_check", "SHUTTING DOWN");
			ShutdownService::shutdown(ShutdownReason::Battery);
			return;
		}
		ESP_LOGI("init_bat_check", "OK");

		battery->OnLevelChanged.bind(this, &ButterBotController::onBatteryChange);
		battery->begin();

		const HardwareConfiguration* config = registerSingleton<HardwareConfiguration>();
		GPIOPeriph* gpio = registerPeriphery<GPIOPeriph>();
		InputGPIO* inputGPIO = registerDriver<InputGPIO>(config->getGPIOInputs(), gpio);
		OutputPWM* outputPWM = registerDriver<OutputPWM>(config->getPwmOutputs());

		static const std::vector<std::pair<Enum<int>, InputPin>> buttonInputs = {
			{ Button::Joystick, { inputGPIO, BTN_JOY } },
			{ Button::ManualOverride, { inputGPIO, BTN_MANOVER } },
			{ Button::Poke, { inputGPIO, BTN_POKE } },
			{ Button::ShutUp, { inputGPIO, BTN_SHUTUP } },
			{ Button::Summon, { inputGPIO, BTN_SUMMON } },
		};

		ButtonInput* buttonInput = registerService<ButtonInput>(buttonInputs, false);

		static const std::vector<std::pair<LEDs, OutputPin>> ledPins = {
			{ LEDs::BigGreen, { outputPWM, (int)LEDs::BigGreen } },
			{ LEDs::Wifi, { outputPWM, (int)LEDs::Wifi } },
			{ LEDs::Power, { outputPWM, (int)LEDs::Power } },
			{ LEDs::BatteryLow, { outputPWM, (int)LEDs::BatteryLow } },
			{ LEDs::TftBacklight, { outputPWM, (int)LEDs::TftBacklight } },
		};

		LED<LEDs, RGB_LEDs>* ledService = registerService<LED<LEDs, RGB_LEDs>>(true);
		ledService->reg(ledPins);

		auto* ledController = registerService<LEDController>(ledService);
		ledController->powerLedOn();

		Display* display = registerDevice<Display>(config->getDisplayBusConfig(), config->getDisplayPanelConfig(), [](Sprite& canvas) {
			canvas.setPsram(true);
			canvas.setColorDepth(lgfx::rgb565_2Byte);
			canvas.createSprite(128, 128);
		});
		display->getLGFX().setSwapBytes(true);

		// Clear screen
		display->getCanvas().fillScreen(TFT_BLACK);
		display->commit();

		ledService->on(LEDs::TftBacklight, 1.0f);

		Joystick* joystick = registerDevice<Joystick>((gpio_num_t)JOY_H, (gpio_num_t)JOY_V, false);

		auto bt = new Bluetooth();
		auto gap = registerSingleton<BLE::GAP>();
		auto client = new BLE::Client(gap);

		auto* robotState = registerService<RobotState>();

		// GAP connected means devices are paired/connected, Com event means service is connected
		auto com = registerService<Com>(client, false);

		com->onData.bind(this, [robotState](BB::State state, BB::Action action, std::vector<uint8_t> data) {
			// BatteryLevel is a status side-channel, not an action; consume it and leave idle tracking untouched
			if(state == BB::State::Idle && action.idle == BB::Action::Idle::BatteryLevel){
				if(data.size() != sizeof(BatteryLevelData)){
					return;
				}

				auto levelData = (BatteryLevelData*)data.data();
				robotState->setBotBatteryLevel(levelData->level);
				robotState->setChargeState(levelData->charge);
				return;
			}

			if(state == BB::State::Idle && action.idle == BB::Action::Idle::ShutUp){
				if(data.size() != sizeof(ShutUpData)){
					return;
				}

				const auto shutUpData = (ShutUpData*)data.data();
				robotState->setMuted(shutUpData->muted);
				return;
			}

			robotState->setIdleNone(state == BB::State::Idle && action.idle == BB::Action::Idle::None);
		});

		Settings* settings = registerService<Settings>();
		if(settings != nullptr){
			ledService->on(LEDs::TftBacklight, settings->get().screenBrightness);
			com->setSensorCommand(sensorModeToCommand(settings->getSensorMode()));
		}

		if(!SPIFFS::init()){
			return;
		}

		auto* themeService = registerService<ThemeService>(settings != nullptr ? settings->get().currentTheme : Theme::Main);
		WiFi* wifi = registerPeriphery<WiFi>();
		WiFiAccessPoint* ap = registerPeriphery<WiFiAccessPoint>(wifi);

		registerService<BatteryIndicator>(battery, ledService);

		registerService<ShutdownService>(true);

		LVGL* lvgl = registerService<LVGL>(display);

		FSLVGL* fslvgl = registerService<FSLVGL>('S', themeService->getCache());

		registerService<JoystickInputLVGL>(joystick, buttonInput, Enum<int>(Button::Joystick));

		lvgl->startScreen([]() {
			return std::make_unique<IntroScreen>();
		});
		lvgl->startThread();
	}

private:
	void onBatteryChange(const Battery::Level level){
		if(level != Battery::Level::Critical){
			return;
		}

		ShutdownService::shutdown(ShutdownReason::Battery);
		vTaskDelay(portMAX_DELAY);
	}
};

CMF_MAIN(ButterBotController)
