#include <Base64.h>

#include <Crypto.h>
#include <Curve25519.h>
#include <RNG.h>
#include <string.h>

void setup() {
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("START");
  
//  uint8_t f[32];
//  uint8_t k[32];
//  Curve25519::dh1(k, f);
//
//  String aux = "";
//  for(uint8_t i = 0; i< 32; i++){
//    aux += char(k[i]);
//    Serial.print(k[i]);
//    Serial.print(".");
//  }
//  Serial.println();
//  Serial.println(aux.length());
//  Serial.println("---------");
//  for(uint8_t i = 0; i< 32; i++){
//    Serial.print(f[i]);
//    Serial.print(".");
//  }
//  Serial.println();
//  Serial.println("---------");
  uint8_t arduino_pri_key[] = {144,101,59,70,136,244,122,44,88,22,62,205,150,141,37,62,251,64,31,68,186,139,253,167,145,27,113,166,0,155,12,98};
  uint8_t arduino_pub_key[] = {197,132,126,249,217,166,188,252,124,30,86,194,152,40,255,62,227,54,72,78,112,57,15,254,38,239,12,106,234,22,252, 50};
  uint8_t smartphone_pub_key[] = {212, 135, 52, 247, 165, 158, 251, 34, 35, 43, 29, 118, 99, 136, 201, 129, 169, 140, 208, 47, 51, 61, 69, 131, 15, 195, 85, 17, 91, 89, 42, 76};
  char to_base_64[32];
  
  Curve25519::dh2(smartphone_pub_key, arduino_pri_key);
  for(uint8_t i = 0; i< 32; i++){
    Serial.print(char(smartphone_pub_key[i]));
    Serial.print(".");
    to_base_64[i] = char(smartphone_pub_key[i]);
  }
  Serial.println();
  for(uint8_t i = 0; i< 32; i++){
    Serial.print(smartphone_pub_key[i]);
    Serial.print(".");
  }
  
  int encodedLen = base64_enc_len(32);
  char encoded[encodedLen];
  base64_encode(encoded, to_base_64, 32);
  Serial.println();
  Serial.println(encoded);
}

void loop() {}
