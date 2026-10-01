#include "PairingScreen.h"

#include <LV_Interface/LVGL.h>
#include <LV_Interface/LVGIF.h>

#include "HomeScreen.h"
#include "Services/ThemeService.h"

PairingScreen::PairingScreen(){
	const auto app = Application::getApp();
	gap = app->getSingleton<BLE::GAP>();
	com = app->getService<Com>();
	ledController = app->getService<LEDController>();

	// Blink Wifi LED
	ledController->wifiLedStrobe();
	ledController->bigGreenLedStrobe();

	// Load the pairing GIF archive
	app->getService<ThemeService>()->activatePairingAssets();

	// Init pairing GIF
	LVGIF* pairingGif = new LVGIF(*this, "S:/pairing");
	pairingGif->setLooping(LVGIF::LoopType::On);
	pairingGif->reset();

	gap->onConnEvent.bind(app->getService<LVGL>(), [this](const BLE::GAP::ConnEvent event) {
		// If connection failed, try again
		if(event == BLE::GAP::ConnEvent::Failed){
			gap->connect();
		}
	});

	com->onConnStatus.bind(app->getService<LVGL>(), [this](const Com::ConnStatus status) {
		// If connection successful, turn on LED and transition to home screen
		if(status == Com::ConnStatus::Connected){
			ledController->wifiLedOn();
			ledController->bigGreenLedOff();
			transition([]() {
				return std::make_unique<HomeScreen>();
			});
		}
	});

	if(!gap->isConnected() && !gap->isConnecting()) gap->connect();
}

PairingScreen::~PairingScreen(){
	LVGL* lvgl = Application::getApp()->getService<LVGL>();

	gap->onConnEvent.unbind(lvgl);
	com->onConnStatus.unbind(lvgl);
}

void PairingScreen::loop(){
	// (Re)connect whenever BLE is idle. A link can drop while it is still being set up - e.g. the controller was
	// restarted while connected and the robot still holds the previous link - and nothing else retries then.
	if(!gap->isConnected() && !gap->isConnecting()){
		gap->connect();
	}
}
