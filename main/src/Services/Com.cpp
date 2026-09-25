#include "Com.h"
#include "BLE/GAP.h"
#include <Core/Application.h>

static constexpr const char* TAG = "Com";

Com::Com(BLE::Client* client, bool internalStack) : AsyncEntity(10, 3 * 1024, CONFIG_CMF_ASYNCENTITY_THREAD_PRIORITY, CONFIG_CMF_ASYNCENTITY_CPU_CORE, internalStack){
	service = client->addService(ServiceUID);
	rxChar = service->addChar(TxCharUID, ESP_GATT_CHAR_PROP_BIT_NOTIFY);
	txChar = service->addChar(RxCharUID, ESP_GATT_CHAR_PROP_BIT_WRITE);

	service->setOnConnectCb([this]() {
		sensorSyncPending = true;
		status = ConnStatus::Connected;
		onConnStatus.broadcast(ConnStatus::Connected);
	});

	service->setOnDisconnectCb([this]() {
		status = ConnStatus::Disconnected;
		onConnStatus.broadcast(ConnStatus::Disconnected);
	});

	rxChar->setOnConnectedCb([this]() {
		rxChar->writeDescr(ESP_GATT_UUID_CHAR_CLIENT_CONFIG, { 0x01, 0x00 });
	});

	txBuf.reserve(BufSize);
}

void Com::setSensorCommand(Ctrl::Command command){
	sensorCommand = command;
	if(status == ConnStatus::Connected){
		sendCommand(command); // called from the UI thread, like the other send* calls
	}
	// On every (re)connect the Com thread re-sends the current value (see tick)
}

Com::ConnStatus Com::getStatus() const{
	return status;
}

void Com::sendCommand(Ctrl::Command command){
	txBuf.resize(sizeof(Ctrl::Command));
	memcpy(txBuf.data(), &command, sizeof(Ctrl::Command));

	txChar->write(txBuf);
	txBuf.clear();
}

void Com::sendDrive(DriveData data){
	Ctrl::Command command = Ctrl::Drive;
	txBuf.resize(sizeof(Ctrl::Command));
	memcpy(txBuf.data(), &command, sizeof(Ctrl::Command));

	txBuf.resize(txBuf.size() + sizeof(DriveData));
	memcpy(txBuf.data() + sizeof(Ctrl::Command), &data, sizeof(DriveData));

	txChar->write(txBuf);
	txBuf.clear();
}

void Com::sendRC(RCData data){
	Ctrl::Command command = Ctrl::EnterRC;
	txBuf.resize(sizeof(Ctrl::Command));
	memcpy(txBuf.data(), &command, sizeof(Ctrl::Command));

	txBuf.resize(txBuf.size() + sizeof(RCData));
	memcpy(txBuf.data() + sizeof(Ctrl::Command), &data, sizeof(RCData));

	txChar->write(txBuf);
	txBuf.clear();
}

void Com::sendScenario(BB::Action::Scenario scenario, ScenarioData data){
	Ctrl::Command command = Ctrl::Scenario;
	txBuf.resize(sizeof(Ctrl::Command));
	memcpy(txBuf.data(), &command, sizeof(Ctrl::Command));

	txBuf.resize(txBuf.size() + sizeof(BB::Action::Scenario));
	memcpy(txBuf.data() + sizeof(Ctrl::Command), &scenario, sizeof(BB::Action::Scenario));

	txBuf.resize(txBuf.size() + sizeof(ScenarioData));
	memcpy(txBuf.data() + sizeof(Ctrl::Command) + sizeof(BB::Action::Scenario), &data, sizeof(ScenarioData));

	txChar->write(txBuf);
	txBuf.clear();
}

void Com::tick(float deltaTime) noexcept{
	if(status != ConnStatus::Connected) return;

	auto gap = Application::getApp()->getSingleton<BLE::GAP>();
	if(!gap->isConnected() || !service->populated() || !rxChar->connected() || !txChar->connected()) return; // TODO: Stop service on disconnect, start on reconnect

	if(sensorSyncPending.exchange(false)){
		// Local buffer: txBuf is used by the UI thread
		const Ctrl::Command cmd = sensorCommand.load();
		std::vector<uint8_t> buf(sizeof(Ctrl::Command));
		memcpy(buf.data(), &cmd, sizeof(Ctrl::Command));
		txChar->write(buf);
	}

	auto notif = rxChar->getNextNotif(portMAX_DELAY);
	if(!notif || notif->data.empty()) return;

	const auto buf = notif->data;
	if(buf.size() < sizeof(BB)){
		ESP_LOGW(TAG, "Received notif with data size less than minimum");
		return;
	}

	BB data{};
	memcpy(&data, buf.data(), sizeof(BB));

	if(buf.size() > sizeof(BB) + data.dataSize){
		ESP_LOGW(TAG, "Received notif data bigger than expected");
	}

	if(data.dataSize == 0){
		onData.broadcast(data.state, data.action, { });
	} else{
		onData.broadcast(data.state, data.action, { buf.data() + sizeof(BB), buf.data() + sizeof(BB) + data.dataSize });
	}
}
