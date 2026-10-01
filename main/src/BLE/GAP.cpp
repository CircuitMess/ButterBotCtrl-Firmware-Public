#include "GAP.h"
#include "Client.h"
#include "ConMan.h"
#include <esp_log.h>
#include <esp_gatt_common_api.h>

static const char* TAG = "BLE::GAP";

BLE::GAP* BLE::GAP::self = nullptr;

BLE::GAP::GAP(){ // TODO: send failed event if connection failed (after a successful scan)
	self = this;

	esp_ble_gatt_set_local_mtu(500);

	esp_ble_gap_register_callback([](esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param){
		if(self == nullptr) return;
		self->ble_GAP_cb(event, param);
	});

	initSecure();
	esp_ble_gap_config_local_privacy(true);
}

BLE::GAP::~GAP(){
	self = nullptr;
	esp_ble_gap_register_callback([](esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param){});
}

void BLE::GAP::connect(){
	if(state != Idle) return;

	state = Scanning;
	result.found = false;

	esp_ble_scan_params_t ScanParams = {
			.scan_type = BLE_SCAN_TYPE_ACTIVE,
			.own_addr_type = BLE_ADDR_TYPE_RPA_PUBLIC,
			.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
			.scan_interval = ESP_BLE_GAP_SCAN_ITVL_MS(50),
			.scan_window = ESP_BLE_GAP_SCAN_WIN_MS(30),
			.scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE
	};
	ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&ScanParams));
}

void BLE::GAP::setClient(Client* client){
	this->client = client;
}

bool BLE::GAP::isConnected(){
	return state == Connected;
}

bool BLE::GAP::isConnecting(){
	return state == Scanning || state == Stopping || state == Connecting;
}

void BLE::GAP::scanResult(const esp_ble_gap_cb_param_t& gap_res){
	const auto& res = gap_res.scan_rst;
	if(res.search_evt != ESP_GAP_SEARCH_INQ_RES_EVT) return;
	if(state != Scanning) return;

	uint8_t* advName = nullptr;
	uint8_t advLen = 0;
	advName = esp_ble_resolve_adv_data_by_type((uint8_t*) res.ble_adv, res.adv_data_len + res.scan_rsp_len, ESP_BLE_AD_TYPE_NAME_CMPL, &advLen);

	if(advName != nullptr && strncmp((char*) advName, Name, (size_t) std::min(advLen, (uint8_t) strlen(Name))) == 0){
		ESP_LOGI(TAG, "Scan result, device "ESP_BD_ADDR_STR", name len %u", ESP_BD_ADDR_HEX(res.bda), advLen);
		ESP_LOG_BUFFER_CHAR(TAG, advName, advLen);

		memcpy(result.addr, res.bda, ESP_BD_ADDR_LEN);
		result.type = res.ble_addr_type;
		result.found = true;

		state = Stopping;
		esp_ble_gap_stop_scanning();
	}
}

void BLE::GAP::scanDone(){
	if(!result.found){
		onConnEvent.broadcast(ConnEvent::Failed);
		state = Idle;
		return;
	}

	ESP_LOGI(TAG, "Scan done. BB found. Starting connection...");
	state = Connecting;

	esp_ble_gattc_open(client->getIF(), result.addr, result.type, true);
}

void BLE::GAP::ble_GAP_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param){
	ESP_LOGV(TAG, "GAP_EVT, event %d", event);
	// TODO: handle ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT -> contains connection parameters (min & max interval, etc.)

	switch(event){
		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
			esp_ble_gap_start_scanning(10);
			break;

		case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
			ESP_LOGI(TAG, "BLE scan started, status: %d", param->scan_start_cmpl.status);
			// A scan that never started would leave the GAP in Scanning forever - report it, so it's retried
			if(param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS && state == Scanning){
				ESP_LOGW(TAG, "BLE scan failed to start: %d", param->scan_start_cmpl.status);
				state = Idle;
				onConnEvent.broadcast(ConnEvent::Failed);
			}
			break;

		case ESP_GAP_BLE_SCAN_RESULT_EVT:
			if(param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT){
				ESP_LOGI(TAG, "BLE scan result -- scan end", param->scan_rst.search_evt);
				scanDone();
			}else if(param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT){
				scanResult(*param);
			}

			break;

		case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
			ESP_LOGI(TAG, "BLE scan stop, status: %d", param->scan_stop_cmpl.status);
			scanDone();
			break;

		case ESP_GAP_BLE_SCAN_TIMEOUT_EVT: // Event not implemented in IDF -- see ESP_GAP_BLE_SCAN_RESULT_EVT
			ESP_LOGI(TAG, "BLE scan timeout");
			break;

		case ESP_GAP_BLE_NC_REQ_EVT:
			/* The app will receive this evt when the IO has DisplayYesNO capability and the peer device IO also has DisplayYesNo capability.
			show the passkey number to the user to confirm it with the number displayed by peer device. */
			esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true); // true for accept, false for not accept
			ESP_LOGI(TAG, "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number:%lu", param->ble_security.key_notif.passkey);
			break;

		case ESP_GAP_BLE_SEC_REQ_EVT:
			/* send the positive(true) security response to the peer device to accept the security request.
			If not accept the security request, should send the security response with negative(false) accept value*/
			esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
			break;

		case ESP_GAP_BLE_AUTH_CMPL_EVT:{
			if(!param->ble_security.auth_cmpl.success){
				ESP_LOGI(TAG, "fail reason = 0x%x", param->ble_security.auth_cmpl.fail_reason);
				break;
			}
			ESP_LOGI(TAG, "paired");
			esp_log_buffer_hex("addr", param->ble_security.auth_cmpl.bd_addr, ESP_BD_ADDR_LEN);

			onConnEvent.broadcast(ConnEvent::Connected);
			state = Connected;

			if(client){
				client->onPairDone();
			}

			break;
		}

		default:
			break;
	}
}

void BLE::GAP::initSecure(){
	uint32_t passkey = 123456; // static passkey
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));

	esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND; // bonding with peer device after authentication
	esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));

	esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE; // set the IO capability to No output No input
	esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));

	uint8_t key_size = 16; // the key size should be 7~16 bytes
	esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));

	uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
	esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));

	uint8_t oob_support = ESP_BLE_OOB_DISABLE;
	esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));

	/* If your BLE device acts as a Slave, the init_key means you hope which types of key of the master should distribute to you,
	and the response key means which key you can distribute to the master;
	If your BLE device acts as a master, the response key means you hope which types of key of the slave should distribute to you,
	and the init key means which key you can distribute to the slave. */

	uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));

	uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}
