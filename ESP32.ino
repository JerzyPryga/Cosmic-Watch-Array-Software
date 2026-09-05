#include "ESP32_func.h"

void setup() {

  /*-----------------------Serial communication begin---------------------*/

  Serial.begin(9600);
  TeensyComm.begin(115200, SERIAL_8N1, 16, 17);

  Serial.println("ESP32 ready!");
}

/*-----------------------Main LOOP---------------------*/

void loop() {

  if (ReadTeensy()) {

    if (MESSAGE["command"] == "NTP") { // trying to obtain current time
      RESPONSE.clear();
      RESPONSE["time_t"] = GetNTPtime();

      RESPONSE["age"] = micros() - age_start;
      serializeJson(RESPONSE, TeensyComm);
      TeensyComm.flush();
      MESSAGE.clear();
    }

    if (MESSAGE["command"] == "CONNECT") { // trying to connect to local network
      RESPONSE = WiFiTry(MESSAGE["WiFi_ID"], MESSAGE["WiFi_password"]);

      serializeJson(RESPONSE, TeensyComm);
      TeensyComm.flush();
      MESSAGE.clear();
    }

    if (MESSAGE["command"] == "REQUEST" && CONNECTED) { // sending HTTP requests
      RESPONSE = WiFiHTTPrequest(MESSAGE["method"], MESSAGE["host"], MESSAGE["url"], MESSAGE["headers"], MESSAGE["body"], MESSAGE["port"]);

      Serial.println((String) RESPONSE["comm"]);

      serializeJson(RESPONSE, TeensyComm);
      TeensyComm.flush();
      MESSAGE.clear();
    }

    else { // sending back message to confirm communication
      serializeJson(MESSAGE, TeensyComm);
      TeensyComm.flush();
      MESSAGE.clear();
    }
  }
}
