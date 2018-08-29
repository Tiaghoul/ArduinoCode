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
const int pin_accept = 7;
const int pin_decline = 2;
char *smartphoneIDsArray[2] = {"uXUi27eQpTCOaB8DfHgD", "randomkeyvalue"};
bool received_smartphone_pub_key = false;

// ECDH arduino public and private keys, smartphone public key.
// The smartphone public key array is later used to store the shared key
uint8_t arduino_secret_key[32];
uint8_t arduino_public_key[32];
uint8_t smartphone_public_key[32];

void blink(int pin_out) {
	digitalWrite(pin_out, HIGH);
	delay(500);
	digitalWrite(pin_out, LOW);
}

void dealWithDisconnection(){
	received_smartphone_pub_key = false;
}

void generateSharedSecret(){
	Serial.println(F("Generating shared secret."));
	Curve25519::dh2(smartphone_public_key, arduino_secret_key);

	for(int i = 0; i< 32; i++){
		Serial.print(smartphone_public_key[i]);
		Serial.print(F(", "));
	}
	Serial.println();
}

void generateECDHkeyValues(){
	Serial.println(F("Gen ECDH."));
	Curve25519::dh1(arduino_public_key, arduino_secret_key);
}

void sendArduinoPubKey(){
	Serial.println(F("Sending pub key."));
	char to_base_64[32];
	for(int i = 0; i<32; i++){
		to_base_64[i] = char(arduino_public_key[i]);
	}

	int encodedLen = base64_enc_len(32);
// encodedLen+1 due to '\0' that base64_encode puts at the end 
	char encoded[encodedLen+1];
	base64_encode(encoded, to_base_64, 32);
	altSerial.println(encoded);
}

int dealWithSmartphoneKey(char* sp_pub_key, int sp_pub_key_size){
	Serial.println(F("Dealing with sp pub key."));
	int decodedLen = base64_dec_len(sp_pub_key, sp_pub_key_size);
	if(decodedLen != 32){
		return -1;
	}
// decodedLen+1 due to '\0' that base64_decode puts at the end 
	char decoded[decodedLen+1];
	base64_decode(decoded, sp_pub_key, sp_pub_key_size);
	for(int kk = 0; kk < decodedLen; kk++){
		smartphone_public_key[kk] = decoded[kk];
	}
	return 0;
}

void dealWithEncryptedMsg(char* enc_msg, int enc_msg_size){
	Serial.println(F("Decrypting message."));
	uint8_t iv[12];
	int decodedLen = base64_dec_len(enc_msg, enc_msg_size);
	char decoded[decodedLen+1];
	base64_decode(decoded, enc_msg, enc_msg_size);
	for(int j=0; j<12; j++){
		iv[j] = decoded[j];
	}
	uint8_t outputBuffer[20];
	GCM<AES256> gcm;
	gcm.setKey(smartphone_public_key, 32);
	gcm.setIV(iv, 12);
	gcm.decrypt(outputBuffer, &decoded[12], decodedLen);
	char * int_to_char = (char *) outputBuffer;
// null-terminate string
	int_to_char[20] = 0;
	Serial.println(int_to_char);
	bool found_match = false;
	for(int i = 0; i < 2; i++){
		if(strcmp(int_to_char,smartphoneIDsArray[i]) == 0){
//			Serial.println(F("found match!"));
			found_match = true;
			blink(pin_accept);  
			break;
		}
	}
	if(!found_match){
		blink(pin_decline);
	}
	dealWithDisconnection();
	generateECDHkeyValues();
}


void dealWithData() {
	char buffer[MAX_MSG_LEN];
	int bufferIdx = 0;
	while (true) {
		if (!altSerial.available())
			continue;

		char single_char = char(altSerial.read());
		// check for new line
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
	// check if bufferidx < 43
	// every data received must be at least 44/45 chars long
	if(bufferIdx < 43){
		Serial.println(F("short string.."));
	//if it already had sent its public key, generate a new key pair
		if(received_smartphone_pub_key){
			generateECDHkeyValues();
			dealWithDisconnection();
		}
		return;
	}
	if (!received_smartphone_pub_key) {
		int return_value = dealWithSmartphoneKey(&buffer[0], bufferIdx-1);
		if(return_value == -1){
			Serial.println(F("unvalid key len.."));
			dealWithDisconnection();
			return;
		}
		received_smartphone_pub_key = true;
		sendArduinoPubKey();
		generateSharedSecret();
		Serial.println(F("sending ok.."));
		altSerial.println(F("okayy"));
		altSerial.flush();
	}
	else {
		dealWithEncryptedMsg(&buffer[0], bufferIdx-1);
	}
	while(altSerial.available() > 0){
		altSerial.read();
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

