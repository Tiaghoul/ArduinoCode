#define MAX_MSG_LEN 100

#include <AltSoftSerial.h>
#include <Base64.h>
#include <Arduino.h>
#include <RNG.h>
#include <string.h>
#include <Crypto.h>
#include <Curve25519.h>
#include <AES.h>
#include <GCM.h>

// no message sent to the arduino can ever have more than this many bytes
// used to assume size of internal buffers
AltSoftSerial altSerial;
const int pin_accept = 13;
const int pin_decline = 8;

String smartphonesArray[2] = {"uXUi27eQpTCOaB8DfHgD", "randomkeyvalue"};
String string_received = "";
String string_ok = "ok";
bool can_read = true;
bool received_smartphone_pub_key = false;
bool received_encrypted_msg = false;
String full_encoded_msg = "";
uint8_t two_halfs = 0;

// ECDH arduino public and private keys, smartphone public key.
// The smartphone public key array is later used to store the shared key
uint8_t arduino_secret_key[32];
uint8_t arduino_public_key[32];
uint8_t smartphone_public_key[32];

void blink(int pin_out) {
	digitalWrite(pin_out, HIGH);
	delay(250);
	digitalWrite(pin_out, LOW);
	delay(250);
	digitalWrite(pin_out, HIGH);
	delay(250);
	digitalWrite(pin_out, LOW);
}

void dealWithDisconnection(){
	received_smartphone_pub_key = false;
	received_encrypted_msg = false;
	memset(arduino_secret_key, 0, sizeof(arduino_secret_key));
	memset(arduino_public_key, 0, sizeof(arduino_public_key));
	memset(smartphone_public_key, 0, sizeof(smartphone_public_key));
}

void generateSharedSecret(){
	//Serial.print(F("Generating shared secret: "));
	Curve25519::dh2(smartphone_public_key, arduino_secret_key);
	/*for(uint8_t i = 0; i< 32; i++){
		Serial.print(smartphone_public_key[i]);
		Serial.print(F(", "));
	}
	Serial.println();*/
}

void generateECDHkeyValues(){
	//Serial.print(F("ECDH: "));
	Curve25519::dh1(arduino_public_key, arduino_secret_key);
	/*
	for(int j = 0; j<32; j++){
		Serial.print(arduino_secret_key[j]);
		Serial.print(F(", "));
	}
	Serial.println();
	
	for(int j = 0; j<32; j++){
		Serial.print(arduino_public_key[j]);
		Serial.print(F(", "));
	}
	Serial.println();
	*/
}

void sendArduinoPubKey(){
	//Serial.println(F("Sending pub key.."));
	char to_base_64[32];
	for(uint8_t i = 0; i<32; i++){
		to_base_64[i] = char(arduino_public_key[i]);
	}
	int encodedLen = base64_enc_len(32);
	char encoded[encodedLen];
	base64_encode(encoded, to_base_64, 32);
	//Serial.println(encoded);	
	altSerial.println(encoded);
}

void dealWithSmartphoneKey(char* sp_pub_key){
	//base64_decode((char*)smartphone_public_key, string_array, sp_pub_key_length);
	//Serial.println(F("Dealing with sp pub key"));
	int sp_pub_key_length = strlen(sp_pub_key);
	int decodedLen = base64_dec_len(sp_pub_key, sp_pub_key_length);
	char decoded[decodedLen];
	base64_decode(decoded, sp_pub_key, sp_pub_key_length);
	for(uint8_t kk = 0; kk < decodedLen; kk++){
		smartphone_public_key[kk] = decoded[kk];
		//Serial.print(smartphone_public_key[kk]);
		//Serial.print(F(", "));
	}
	//Serial.println();

}

void dealWithEncryptedMsg(String enc_msg){
	//Serial.print(F("Received encypted message: "));
	//Serial.println(enc_msg);

	uint8_t iv[12];
	uint8_t encodedTextSize = enc_msg.length();
	char encodedTextArray[100];
	enc_msg.toCharArray(encodedTextArray, encodedTextSize+1);
	uint8_t decodedLen = base64_dec_len(encodedTextArray, encodedTextSize);
	char decoded[decodedLen];
	base64_decode(decoded, encodedTextArray, encodedTextSize);
	//Serial.println(decodedLen);
	uint8_t cipheredTextValues[decodedLen-12];
	//Serial.print(F("IV: "));
	for(uint8_t j=0; j<12; j++){
		iv[j] = decoded[j];
		//Serial.print(iv[j]);
		//Serial.print(F(", "));
	}
	//Serial.println();
	//Serial.println(F("CIPHERED TEXT: "));
	for(uint8_t k = 12; k < decodedLen; k++){
		cipheredTextValues[k-12] = decoded[k];
		//Serial.print(cipheredTextValues[k-12]);
		//Serial.print(", ");
	}
	//Serial.println();
	uint8_t outputBuffer[20];
	GCM<AES256> gcm;
	gcm.setKey(smartphone_public_key, sizeof(smartphone_public_key));
	gcm.setIV(iv, sizeof(iv));
	// technically don't need to use any intermediate memory and can just do:
	// gcm.setIV(decoded, 12);
	gcm.decrypt(outputBuffer, cipheredTextValues, decodedLen);
	// one of these also works:
	// gcm.decrypt(outputBuffer, decoded + 12, decodedLen);
	// gcm.decrypt(outputBuffer, &decoded[12], decodedLen);
	for(uint8_t k = 0; k<sizeof(outputBuffer); k++){
		//Serial.print(char(outputBuffer[k]));
	}
	//Serial.println();
	//dealWithDisconnection();
}

/*
void dealWithData() {
	while (altSerial.available() > 0) {
		char char_received = altSerial.read();
		//Serial.println(String("----> ") + char_received + String(" || ") + int(received_smartphone_pub_key));
		if (char_received == '\n') {
			Serial.println("-> ." + string_received + ".");
			if (received_smartphone_pub_key) {
				Serial.println("true");
			}
			else {
				Serial.println("false");
			}
			if(!received_smartphone_pub_key){
				received_smartphone_pub_key = true;
				//Serial.println(String("ns: ") + int(received_smartphone_pub_key));
				sendArduinoPubKey();
				dealWithSmartphoneKey(string_received.substring(0, string_received.length() - 1));
				generateSharedSecret();
				Serial.println(F("sending ok to ble nano2.."));
				altSerial.println(F("okay2"));
				altSerial.flush();
			}
			else {
				Serial.println(F("gottem"));
				Serial.println(string_received);
				//dealWithEncryptedMsg(string_received);
				//generateECDHkeyValues();
				Serial.println(F("gottem2"));
			}
			string_received = "";
		}
		else {
			string_received += char_received;
		}
	}
}
*/


void dealWithData2() {
	char buffer[MAX_MSG_LEN];
	int bufferIdx = 0;
	// limpar o serial a seguir ao "break"
	while (true) {
		if (!altSerial.available())
			continue;

		char single_char = char(altSerial.read());
		// check for new line or carriage return
		if (single_char == '\n') 
			break;
		
		buffer[bufferIdx] = single_char;
		bufferIdx += 1;
	}

	// null-terminate string
	buffer[bufferIdx] = 0;
	//String char_to_string = String(buffer);
	Serial.print(F("Recv: "));
	Serial.println(buffer);
	if (!received_smartphone_pub_key) {
		received_smartphone_pub_key = true;
		sendArduinoPubKey();
		dealWithSmartphoneKey(buffer);
		generateSharedSecret();
		Serial.println(F("sending ok.."));
		altSerial.println(F("okayy"));
		altSerial.flush();
	}
	else {
		Serial.println(buffer);
		Serial.println(F("gttm"));
	}

}

void setup() {
	Serial.begin(57600);
	pinMode(pin_accept, OUTPUT);
	pinMode(pin_decline, OUTPUT);

	while (!Serial) {;}
	Serial.println(F("hi!"));
	
	generateECDHkeyValues();
	altSerial.begin(9600);
}

void loop() {
	if (altSerial.available() > 0) {
		dealWithData2();
	}
}

