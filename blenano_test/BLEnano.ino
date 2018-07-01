#include <nRF5x_BLE_API.h>

#define DEVICE_NAME       "LED_1"
#define TXRX_BUF_LEN      30
// Create ble instance
BLE                       ble;
// Create a timer task000
Ticker                    ticker1s;

const int pin_out = D13;

void blinky(void) {
	digitalWrite(pin_out, HIGH);
	delay(200);
	digitalWrite(pin_out, LOW);
	delay(200);
	digitalWrite(pin_out, HIGH);
	delay(400);
	digitalWrite(pin_out, LOW);
}

// The uuid of service and characteristics
static const uint8_t service1_uuid[]         = {0x71, 0x3D, 0, 0, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t service1_chars1_uuid[]  = {0x71, 0x3D, 0, 2, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t service1_chars2_uuid[]  = {0x71, 0x3D, 0, 3, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t service1_chars3_uuid[]  = {0x71, 0x3D, 0, 4, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
// Used in advertisement
static const uint8_t uart_base_uuid_rev[]    = {0x1E, 0x94, 0x8D, 0xF1, 0x48, 0x31, 0x94, 0xBA, 0x75, 0x4C, 0x3E, 0x50, 0, 0, 0x3D, 0x71};

// Initialize value of chars0
uint8_t chars_value[TXRX_BUF_LEN] = {0};

// Create characteristic
GattCharacteristic  characteristic1(service1_chars1_uuid, chars_value, 1, TXRX_BUF_LEN, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE );
GattCharacteristic  characteristic2(service1_chars2_uuid, chars_value, 1, TXRX_BUF_LEN, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);
GattCharacteristic  characteristic3(service1_chars3_uuid, chars_value, 1, TXRX_BUF_LEN, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);
GattCharacteristic *customServChars[] = {&characteristic1, &characteristic2, &characteristic3};
//Create service
GattService         customService(service1_uuid, customServChars, sizeof(customServChars) / sizeof(GattCharacteristic *));

//DeviceInformationService *deviceInfo;

void disconnectionCallBack(const Gap::DisconnectionCallbackParams_t *params) {
  //Serial.println("Restart advertising ");
  ble.startAdvertising();
}


void connectionCallBack( const Gap::ConnectionCallbackParams_t *params ) {
  //uint8_t index;
  //Serial.print("The ownAddr type : ");
  //Serial.println(params->ownAddrType, HEX);
  //Serial.print("  The ownAddr : ");
  /*for(index=5; index > 0; index--) {
    Serial.print(params->ownAddr[index], HEX);
    Serial.print(" ");
  }
  Serial.print(params->ownAddr[0], HEX);
  Serial.println(" ");*/
}


void gattServerWriteCallBack(const GattWriteCallbackParams *Handler) {
  uint8_t index;
  String value_written = "";
  String string_received = "";

  for(index=0; index<Handler->len; index++) {
	  value_written += char(Handler->data[index]);
  }
  Serial.println(value_written);

  int n_tries = 20;
  while (!Serial.available()) {
	  blinky();
	  n_tries -= 1;
	  if (n_tries == 0) {
		  char coise[] = "failed";
		  ble.updateCharacteristicValue(characteristic3.getValueAttribute().getHandle(), (uint8_t *)&coise, sizeof(coise)-1);
		  return;
	  }
  }

  while (Serial.available() > 0) {
	  char char_received = Serial.read();
	  if (char_received == '\n') {
		Serial.println(string_received);
		int string_size = string_received.length();
		char* recebido = new char[string_size+1];
		string_received.toCharArray(recebido, string_size+1);
		ble.updateCharacteristicValue(characteristic3.getValueAttribute().getHandle(), (uint8_t *)recebido, string_size);	  
		return;
	  }
	  else {
		  string_received += char_received;
	  }
  }

  char coise[] = "failed";
  ble.updateCharacteristicValue(characteristic3.getValueAttribute().getHandle(), (uint8_t *)&coise, sizeof(coise) - 1);
}


void setAdvertisement(void) {
  ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
  // Add short name to advertisement
  ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,(const uint8_t *)"TXRX", 4);
  // Add complete 128bit_uuid to advertisement
  ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,(const uint8_t *)uart_base_uuid_rev, sizeof(uart_base_uuid_rev));
  // Add complete device name to scan response data
  ble.accumulateScanResponse(GapAdvertisingData::COMPLETE_LOCAL_NAME,(const uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME) - 1);
}

void setup() {
  Serial.begin(9600);	
  //Serial.println(DEVICE_NAME);
  // Init ble
  ble.init();
  ble.gap().onConnection(connectionCallBack);
  ble.gap().onDisconnection(disconnectionCallBack);
  ble.gattServer().onDataWritten(gattServerWriteCallBack);
 // procurar mtu stuff
 
  // set advertisement
  setAdvertisement();
  ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
  ble.addService(customService);
  ble.setDeviceName((const uint8_t *)DEVICE_NAME);
  ble.setTxPower(4);
  ble.setAdvertisingInterval(160);
  ble.setAdvertisingTimeout(0);

  ble.startAdvertising();
}

void loop() {
  ble.waitForEvent();
}

