#include <Base64.h>
#include <Crypto.h>
#include <AES.h>
#include <GCM.h>
#include <string.h>

void setup() {
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("START");
  
  uint8_t iv[12];
  uint8_t key[] = {156, 158, 42, 137, 10, 107, 210, 236, 195, 17, 34, 73, 21, 166, 22, 185, 15, 173, 104, 195, 77, 77, 148, 150, 23, 53, 160, 66, 239, 228, 61, 37};
  String encodedCipheredText = "1WvlEDBrKCsF+NWTguFs83r6OANBJjBX/ijfiqDyLgM7CgbRqJ4A41GLSZpKs1y0";
  int encodedCT_size = encodedCipheredText.length();
  char* encodedCT_array = new char[encodedCT_size +1];
  encodedCipheredText.toCharArray(encodedCT_array, encodedCT_size +1);
  int decodedLen = base64_dec_len(encodedCT_array, encodedCT_size);
  char decoded[decodedLen];
  base64_decode(decoded, encodedCT_array, encodedCT_size);
  Serial.println(decodedLen);
//  Serial.println(decoded);
  uint8_t cipheredTextValues[decodedLen-12];
  Serial.print("IV: ");
  for(uint8_t j = 0; j < 12; j++){
    iv[j] = decoded[j];
    Serial.print(iv[j]);
    Serial.print(", ");
  }
  Serial.println();
  Serial.println("CIPHERED TEXT: ");
  for(uint8_t k = 12; k < decodedLen; k++){
//    uint8_t aux = decoded[k];
    cipheredTextValues[k-12] = decoded[k];
    Serial.print(cipheredTextValues[k-12]);
    Serial.print(", ");
  }
  
  Serial.println();
  uint8_t outputBuffer[20];
  GCM<AES256> gcm;
  gcm.setKey(key, sizeof(key));
  gcm.setIV(iv, sizeof(iv));
  gcm.decrypt(outputBuffer, cipheredTextValues, decodedLen);
  for(uint8_t k = 0; k < sizeof(outputBuffer); k++){
    Serial.print(char(outputBuffer[k]));
//    Serial.print(", ");
  }
  Serial.println();
}

void loop() {
  // put your main code here, to run repeatedly:

}
