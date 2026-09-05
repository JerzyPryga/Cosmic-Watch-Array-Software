#include "ESP8266_functions.h"

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);

  for (int n = 0; n < 3; n++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }

  // /*-----------------------Serial monitor begin---------------------*/

  Serial.begin(9600);
  Serial.setTimeout(50);
  //TeensyComm.begin(9600);
  Serial.println("\n==================================================");

  /*-----------------------Ethernet or WiFi connection attempt + NTP request---------------------*/

  ESP.wdtDisable();
  ESP.wdtEnable(6000);
  CONNECTED = ConnectTry();

  /*-----------------------Set timestamp of start------------------*/

  timestamp_last = Timestamp_Read();  // in [ns]
  time_start = Timestamp_Read();      // in [ns]

  delay(2000);
}

void loop() {

  MESSAGE = Teensy_Read();

  if(MESSAGE != "") {

    if(FindInJSON(MESSAGE, "command") == "LOGIN") {

      user_login = FindInJSON(MESSAGE, "user_login");
      user_password = FindInJSON(MESSAGE, "password");

      if(LOGIN_REQUEST(user_login, user_password)) {
        Serial.print(user_token.c_str());
        Serial.flush();
      }

      else {
        Serial.print(0);
        Serial.flush();
      }
    }

    if(FindInJSON(MESSAGE, "command") == "PING") {

      string time_stamp = FindInJSON(MESSAGE, "time_stamp");
      string delta_time = FindInJSON(MESSAGE, "delta_time");
      string on_time = FindInJSON(MESSAGE, "on_time");

      if(PING_REQUEST(time_stamp, delta_time, on_time)) {
        Serial.print(1);
        Serial.flush();
      }

      else {
        Serial.print(0);
        Serial.flush();
      }
    }

    if(FindInJSON(MESSAGE, "command") == "DATA_SEND") {

      string data_content = FindInJSON(MESSAGE, "data_content");

      if(DATA_REQUEST(data_content)) {
        Serial.print(1);
        Serial.flush();
      }

      else {
        Serial.print(0);
        Serial.flush();
      }
    }

  }
}
