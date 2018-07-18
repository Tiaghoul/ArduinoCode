#include <Base64.h>
#include <SoftwareSerial.h>
#include <Arduino.h>

SoftwareSerial mySerial(10, 11); // RX, TX
const int pin_accept = 13;
//const int pin_decline = 8;

String smartphonesArray[2] = {"uXUi27eQpTCOaB8DfHgD", "deeznuts"};
String string_received = "";
String string_open = "open";
bool can_read = true;
bool received_public = false;
String full_encoded_msg = "";
uint8_t two_halfs = 0;

//public variable representing the shared secret key.
String key_string;
uint32_t k;
uint32_t A;
uint32_t a;
//prime number 
const uint32_t prime = 2147483647;
//generator
const uint32_t generator = 16807;

uint32_t keyGen() {
  //Seed the random number generator with a reading from an unconnected pin, I think this on analog pin 2
  randomSeed(analogRead(2));

  //return a random number between 1 and our prime .
  return random(1, prime);
}

//code to compute the remainder of two numbers multiplied together.
uint32_t mul_mod(uint32_t a, uint32_t b, uint32_t m) {


  uint32_t result = 0; //variable to store the result
  uint32_t runningCount = b % m; //holds the value of b*2^i

  for (int i = 0; i < 32; i++) {

    if (i > 0) runningCount = (runningCount << 1) % m;
    if (bitRead(a, i)) {
      result = (result%m + runningCount % m) % m;

    }

  }
  return result;
}

//The pow_mod function to compute (b^e) % m that was given in the class files  
uint32_t pow_mod(uint32_t b, uint32_t e, uint32_t m) {
  uint32_t r;  // result of this function
  uint32_t pow;
  uint32_t e_i = e;
  uint8_t i;

  if (b == 0 || m == 0) {
    return 0;
  }

  if (e == 0) {
    return 1;
  }

  b = b % m;

  pow = b;

  r = 1;

  while (e_i) {
    if (e_i & 1) {
      r = mul_mod(r, pow, m);//(r * pow) % m; 
    }

    pow = mul_mod(pow, pow, m);//(pow * pow) % m;

    e_i = e_i >> 1;
    i++;
  }

  return r;
}



void blink(int pin_out) {
  digitalWrite(pin_out, HIGH);
  delay(250);
  digitalWrite(pin_out, LOW);
  delay(250);
  digitalWrite(pin_out, HIGH);
  delay(250);
  digitalWrite(pin_out, LOW);
}

void generateKeyValues() {
  a = keyGen();
  A = pow_mod(generator, a, prime);
}

void dealWithSmartphoneKey(String spKey) {
  Serial.println();
  Serial.println(F("---> dealing with smartphone key"));
  uint32_t smartPhoneKey = spKey.substring(0, spKey.length() - 1).toInt();
  if (smartPhoneKey == 0) {
    Serial.println(F("it's 0, returning now"));
    received_public = false;
    mySerial.println("fail1");
    return;
  }
  Serial.print("a = ");
  Serial.println(a);
  Serial.print("A = ");
  Serial.println(A);
  mySerial.println(String(A));
  k = pow_mod(smartPhoneKey, a, prime);
  key_string = String(k);
  Serial.print(F("shared key = "));
  Serial.println(key_string);
  generateKeyValues();
}

void xorDecrypt(String encryptedMessage) {
  two_halfs = 0;
  Serial.println();
  Serial.print(F("---> xoring and decripting: "));
  Serial.println(encryptedMessage);

  
  int input2len = encryptedMessage.length();
  char* string_array = new char[input2len + 1];
  encryptedMessage.toCharArray(string_array, input2len + 1);
  int decodedlen = base64_dec_len(string_array, input2len);
  char decoded[decodedlen];
  base64_decode(decoded, string_array, input2len);
//  Serial.println(decoded);

  String decryptedMessage = "";
  for (uint8_t i = 0; i < decodedlen; i++) {
    //decryptedMessage += char(encryptedMessage.charAt(i) ^ key_string.charAt(i % key_string.length()));
    decryptedMessage += char(decoded[i] ^ key_string.charAt(i % key_string.length()));
  }
  decryptedMessage = decryptedMessage.substring(0, decryptedMessage.length() - 1);
  Serial.print(F("decripted message : "));
  Serial.println(decryptedMessage);
  for (uint8_t i = 0; i < sizeof(smartphonesArray) / sizeof(String); i++) {
    if (smartphonesArray[i].equals(decryptedMessage)){
      Serial.println(F("SUCCESSSSSS"));
      mySerial.println("success");
      blink(pin_accept);
      return;
    }
  }

  Serial.println(F("none of the IDs were right..."));
  mySerial.println("fail2");
//  blink(pin_decline);
}

void dealWithData() {
  while (mySerial.available() > 0) {
    char char_received = mySerial.read();
    if (char_received == '\n') {
      if (!received_public) {
        Serial.println("| " + string_received + " |");
        received_public = true;
        dealWithSmartphoneKey(string_received);
      }
      else if (two_halfs == 0) {
        Serial.println("|| " + string_received + " ||");
        full_encoded_msg = string_received.substring(0, string_received.length() - 1);
        two_halfs += 1;
        mySerial.println("ok");
      }
      else if(two_halfs == 1){
        Serial.println("||| " + string_received + " |||");
        full_encoded_msg += string_received;
        received_public = false;
        xorDecrypt(full_encoded_msg);
      }
      string_received = "";
    }
    else {
      string_received += char_received;
    }
  }
  can_read = true;

}


void setup() {
  Serial.begin(57600);
//  pinMode(pin_accept, OUTPUT);
//  pinMode(pin_decline, OUTPUT);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println(F("START"));
  
  generateKeyValues();

  mySerial.begin(9600);
}

void loop() {
  if (mySerial.available() > 0 && can_read) {
    can_read = false;
    dealWithData();
  }
}


