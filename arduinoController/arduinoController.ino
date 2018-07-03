#define MAX_MSG_LEN 70

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
bool received_smartphone_pub_key = false;
bool received_encrypted_msg = false;

// ECDH arduino public and private keys, smartphone public key.
// The smartphone public key array is later used to store the shared key
uint8_t arduino_secret_key[32];
uint8_t arduino_public_key[32];
uint8_t smartphone_public_key[32];

void blink(int pin_out) {
	digitalWrite(pin_out, HIGH);
	delay(1000);
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
	Serial.println(F("Generating shared secret: "));
	Curve25519::dh2(smartphone_public_key, arduino_secret_key);
/*
	for(uint8_t i = 0; i< 32; i++){
		Serial.print(smartphone_public_key[i]);
		Serial.print(F(", "));
	}
	Serial.println();
*/
}

void generateECDHkeyValues(){
	//Serial.print(F("ECDH: "));
	Curve25519::dh1(arduino_public_key, arduino_secret_key);
	Serial.println(F("done"));
	/*	
	for(int j = 0; j<32; j++){
		Serial.print(arduino_public_key[j]);
		Serial.print(F(", "));
	}
	Serial.println();
	*/
}

void sendArduinoPubKey(){
	Serial.println(F("Sending pub key.."));
	char to_base_64[32];
	for(uint8_t i = 0; i<32; i++){
		to_base_64[i] = char(arduino_public_key[i]);
	}

	int encodedLen = base64_enc_len(32);
// encodedLen+1 due to '\0' that base64_encode puts at the end 
	char encoded[encodedLen+1];
	base64_encode(encoded, to_base_64, 32);
	//Serial.print(F("encoded addr:"));
	//Serial.println((size_t)encoded, HEX);
	altSerial.println(encoded);
}

void dealWithSmartphoneKey(char* sp_pub_key, uint8_t sp_pub_key_size){
	// check if key size is 32, else "dealwithdisconnection"
	Serial.println(F("Dealing with sp pub key"));

	//int sp_pub_key_length = sizeof(sp_pub_key);
	Serial.println(sp_pub_key);
	Serial.println(F("~~~~~~~~"));
	int decodedLen = base64_dec_len(sp_pub_key, sp_pub_key_size);
	char decoded[decodedLen];
	base64_decode(decoded, sp_pub_key, sp_pub_key_size);
	Serial.println(F("----"));
	for(uint8_t kk = 0; kk < decodedLen; kk++){
		smartphone_public_key[kk] = decoded[kk];
		Serial.print(smartphone_public_key[kk]);
		Serial.print(F(", "));
	}
	Serial.println();

}

void dealWithEncryptedMsg(String enc_msg){
	// do the decoding the same way as in "dealWithSmartphoneKey"

	//Serial.print(F("Received encypted message: "));

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


void dealWithData() {
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
	Serial.print(F("Recv: "));
	Serial.println(buffer);
	Serial.println(bufferIdx);
	if (!received_smartphone_pub_key) {
		Serial.print(F("R1: "));
		Serial.println(buffer);
		Serial.print(F("buffer addr:"));
		Serial.println((size_t)buffer, HEX);

		received_smartphone_pub_key = true;
		sendArduinoPubKey();

		Serial.print(F("R2: "));
		Serial.println(buffer);
		dealWithSmartphoneKey(&buffer[0], bufferIdx-1);
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
	Serial.println(F("hi"));
	
	generateECDHkeyValues();
	altSerial.begin(9600);
}

void loop() {
	if (altSerial.available() > 0) {
		dealWithData();
	}
}

