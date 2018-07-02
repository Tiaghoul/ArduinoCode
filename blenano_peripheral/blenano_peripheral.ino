
#include <nRF5x_BLE_API.h>

#define DEVICE_NAME       "NANO 1"
#define TXRX_BUF_LEN      30
// Create ble instance
BLE                       ble;

const int pin_out = D13;
char ok_string1[] = "ok1";

bool received_android_key = false;
bool received_first_half = false;
bool received_arduino_key = false;
uint8_t number_of_key_parts = 0;
uint8_t current_key_part = 0;
String full_android_key = "";
String full_arduino_key = "";

uint8_t number_of_msg_parts = 0;
uint8_t current_msg_part = 0;
String full_encrypted_msg = "";
bool received_encrypted_msg = false;
bool received_first_part_msg = false;

void blinky(void) {
  digitalWrite(pin_out, HIGH);
  delay(250);
  digitalWrite(pin_out, LOW);
  delay(100);
  digitalWrite(pin_out, HIGH);
  delay(250);
  digitalWrite(pin_out, LOW);
  delay(100);
}
// The uuid of service and characteristics
static const uint8_t service1_uuid[] = { 0x71, 0x3D, 0, 0, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E };
static const uint8_t service1_chars1_uuid[] = { 0x71, 0x3D, 0, 2, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E };
static const uint8_t service1_chars2_uuid[] = { 0x71, 0x3D, 0, 3, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E };
static const uint8_t service1_chars3_uuid[] = { 0x71, 0x3D, 0, 4, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E };
// Used in advertisement
static const uint8_t uart_base_uuid_rev[] = { 0x1E, 0x94, 0x8D, 0xF1, 0x48, 0x31, 0x94, 0xBA, 0x75, 0x4C, 0x3E, 0x50, 0, 0, 0x3D, 0x71 };

// Initialize value of chars0
uint8_t chars_value[TXRX_BUF_LEN] = { 0 };

// Create characteristic
GattCharacteristic  characteristic1(service1_chars1_uuid, chars_value, 1, TXRX_BUF_LEN, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE);
GattCharacteristic  characteristic2(service1_chars2_uuid, chars_value, 1, TXRX_BUF_LEN, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);
GattCharacteristic  characteristic3(service1_chars3_uuid, chars_value, 1, TXRX_BUF_LEN, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);
GattCharacteristic *customServChars[] = { &characteristic1, &characteristic2, &characteristic3 };
//Create service
GattService         customService(service1_uuid, customServChars, sizeof(customServChars) / sizeof(GattCharacteristic *));

//DeviceInformationService *deviceInfo;

void dealWithDisconnection(){
  received_arduino_key = false;
  received_android_key = false;
  received_first_half = false;
  received_encrypted_msg = false;
  received_first_part_msg = false;
  number_of_key_parts = 0;
  number_of_msg_parts = 0;
  current_key_part = 0;
  current_msg_part = 0;
  full_encrypted_msg = "";
  full_arduino_key = "";
  full_android_key = "";
  while (Serial.available() > 0) {
    char char_received = Serial.read();
  }
  ble.startAdvertising();
}


// Waiting to receive public key from arduino,
// and send it to smartphone
void wait_for_arduino_key(){
  int n_tries = 20;
  while(!Serial.available()){
    delay(700);
    n_tries -= 1;
    if(n_tries == 0){
      char coise[] = "failed1";
      ble.updateCharacteristicValue(characteristic3.getValueAttribute().getHandle(), (uint8_t *)&coise, sizeof(coise));
      return;
    }
  }
  while (Serial.available() > 0) {
    char char_received = Serial.read();
    if (char_received == '\n') {
      received_arduino_key = true;
      for(uint8_t i=0; i<2; i++){
        String i_part = full_arduino_key.substring(20*i, 20*(i+1));
        char* to_send = new char[21];
        i_part.toCharArray(to_send, 21);
        ble.updateCharacteristicValue(characteristic3.getValueAttribute().getHandle(), (uint8_t *)to_send, 20);
        delay(75); 
      }
      uint8_t string_size = full_arduino_key.length();
      String third_half = full_arduino_key.substring(40, string_size);
      char* last_to_send = new char[string_size - 39];
      third_half.toCharArray(last_to_send, string_size - 39);
      ble.updateCharacteristicValue(characteristic3.getValueAttribute().getHandle(), (uint8_t *)last_to_send, string_size - 40);
      return;
    }
    else {
      full_arduino_key += char_received;
    }
  }
}

//Waiting to receive an "ok" from the Arduino, in order to send the encrypted text from the smartphone
void wait_for_arduino_ok(){
  int n_tries = 20;
  while(!Serial.available()){
    delay(500);
    n_tries -= 1;
    if(n_tries == 0){
      ble.updateCharacteristicValue(characteristic3.getValueAttribute().getHandle(), (uint8_t *)&ok_string1, sizeof(ok_string1));
      dealWithDisconnection();
      return;
    }
  }
  String string_to_send = "";
  while (Serial.available() > 0) {
    char char_received = Serial.read();
    string_to_send += char_received;
  }
  int string_size = string_to_send.length();
  char* string2array = new char[string_size+1];
  string_to_send.toCharArray(string2array, string_size+1);
  delay(40);
  Serial.println(full_encrypted_msg);
  Serial.flush();
  delay(400);
  dealWithDisconnection();
  delay(400);
  ble.updateCharacteristicValue(characteristic3.getValueAttribute().getHandle(), (uint8_t *)string2array, string_size);
}

void disconnectionCallBack(const Gap::DisconnectionCallbackParams_t *params) {
//  Serial.println("Restart advertising ");
  while (Serial.available() > 0) {
    char char_received = Serial.read();
  }
  received_arduino_key = false;
  received_android_key = false;
  received_first_half = false;
  received_encrypted_msg = false;
  received_first_part_msg = false;
  number_of_key_parts = 0;
  number_of_msg_parts = 0;
  current_key_part = 0;
  current_msg_part = 0;
  ble.startAdvertising();
}


void connectionCallBack(const Gap::ConnectionCallbackParams_t *params) {}


void gattServerWriteCallBack(const GattWriteCallbackParams *Handler) {
  uint8_t index;
  String value_written = "";

  for (index = 0; index<Handler->len; index++) {
    value_written += char(Handler->data[index]);
  }

  if(!received_android_key){
    if(!received_first_half){
      char nop = value_written.charAt(0);
      number_of_key_parts = nop - '0';
      received_first_half = true;
      current_key_part = 1;
      full_android_key += value_written.substring(1,value_written.length());
    }
    else{
      full_android_key += value_written;
      current_key_part += 1;
      if(current_key_part == number_of_key_parts){
        received_android_key = true;
        Serial.println(full_android_key);
        full_android_key = "";
        wait_for_arduino_key();
        return;
      }
    }
  }
  else if(!received_encrypted_msg){
    if(!received_first_part_msg){
      char n_msg_parts = value_written.charAt(0);
      number_of_msg_parts = n_msg_parts - '0';
      received_first_part_msg = true;
      current_msg_part = 1;
      full_encrypted_msg += value_written.substring(1, value_written.length());
    }
    else{
      full_encrypted_msg += value_written;
      current_msg_part += 1;
      if(current_msg_part == number_of_msg_parts){
        received_encrypted_msg = true;
        wait_for_arduino_ok();
        return;
      }
    }
  }
}


void setAdvertisement(void) {
  ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
  // Add short name to advertisement
  ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME, (const uint8_t *)"TXRX", 4);
  // Add complete 128bit_uuid to advertisement
  ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS, (const uint8_t *)uart_base_uuid_rev, sizeof(uart_base_uuid_rev));
  // Add complete device name to scan response data
  ble.accumulateScanResponse(GapAdvertisingData::COMPLETE_LOCAL_NAME, (const uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME) - 1);
}

void setup() {
  Serial.begin(9600);
  //Serial.println(DEVICE_NAME);
  pinMode(pin_out, OUTPUT);

  // Init ble
  ble.init();
  ble.gap().onConnection(connectionCallBack);
  ble.gap().onDisconnection(disconnectionCallBack);
  ble.gattServer().onDataWritten(gattServerWriteCallBack);

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


