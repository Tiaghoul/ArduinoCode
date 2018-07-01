#include <Base64.h>

void setup() {
  Serial.begin(9600);
    while (!Serial) {
      ; // wait for serial port to connect. Needed for Leonardo only
    }
  Serial.println("Base64 example");

  char input2[] = "RG1iWQUEUmFGZXJ6VnIPd1F4UXU=";
  int input2len = sizeof(input2);
  int decodedlen = base64_dec_len(input2, input2len);
  char decoded[decodedlen];
  base64_decode(decoded, input2, input2len);
  Serial.println(input2);
  Serial.println(decoded);
  Serial.println("------------");
}

void loop() {
  // put your main code here, to run repeatedly:

}
