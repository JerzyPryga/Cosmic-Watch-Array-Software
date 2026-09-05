#include <chrono>
#include <string>
#ifndef ESP_FUNCTIONS_HH
#define ESP_FUNCTIONS_HH

#include "ESP32_libs.h"

/*----------Attempt Internet conncetion via WiFi-------------*/
JsonDocument WiFiTry(string WiFi_name, string WiFi_pass) {

  Serial.print("WI-FI: ");

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFi_name.c_str(), WiFi_pass.c_str());

  int wait = 0, d_wait = 100;  // [ms]
  while (WiFi.status() != WL_CONNECTED && wait < 5000) {
    delay(d_wait);
    wait = wait + d_wait;
  }

  JsonDocument mess;

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("SUCCCESFULLY connected. ");
    Serial.println(WiFi.localIP());

    mess["err"] = 1;
    mess["comm"] = "SUCCCESFULLY connected. ";
    mess["ip"] = WiFi.localIP();

    CONNECTED = true;
    return mess;
  }

  Serial.print("FAILED to configure Wi-Fi. ");
  mess["err"] = 0;
  mess["comm"] = "FAILED to configure Wi-Fi. ";

  if (WiFi.status() == WL_NO_SSID_AVAIL) {
    Serial.println(("No network named " + WiFi_name + " found.").c_str());
    mess["comm"] = "FAILED to configure Wi-Fi. No network named " + WiFi_name + "found.";
  }

  return mess;
}

/*----------Get time using NTP-------------*/
unsigned long GetNTPtime() {

  RESPONSE["err"] = 0;

  time_client.begin();

  unsigned long timeout = millis();
  while(!time_client.forceUpdate()) {    
    if(millis() - timeout > 5000)
      return 0;
  }

  unsigned long epoch_time = time_client.getEpochTime();
  age_start = micros();

  time_client.end();

  if(epoch_time > 1)
    RESPONSE["err"] = 1;

  return epoch_time;
}

/*----------Find RESPONSE code-------------*/
int FindResponseCode(string resp_str, string &JSON_comm) {

  string str = "0";
  if (resp_str.find("HTTP/1.1 ") < resp_str.length() && resp_str.find("HTTP/1.1 ") >= 0) {

    resp_str = resp_str.substr(resp_str.find("HTTP/1.1 ") + 9);
    if (resp_str.find(" ") < resp_str.find("/n"))
      str = resp_str.substr(0, resp_str.find(" "));
    else
      str = resp_str.substr(0, resp_str.find("\n"));
  }

  else
    JSON_comm.append("No response code found. ");

  return std::stoi(str);
}

/*----------Extract JSON body from HTTP response-------------*/
JsonDocument ReadJSON(string resp_str = "") {

  JsonDocument JSON;
  deserializeJson(JSON, "{\"comm\":0}");

  while(resp_str.find("\n") < resp_str.length() && resp_str.find("\n") >= 0) {
    resp_str = resp_str.substr(resp_str.find("\n") + 1);

    deserializeJson(JSON, resp_str);
  }

  return JSON;
}

/*---------------------Communication with Teensy-------------------*/
int ReadTeensy() {

  if (TeensyComm.available()) {
    deserializeJson(MESSAGE, TeensyComm);

    return 1;
  }

  return 0;
}

/*----------Universal WiFi HTTP request-------------*/
JsonDocument WiFiHTTPrequest(const char *HTTP_METHOD, const char *HOST, const char *URL = "", const char *HEADERS = "", const char *BODY = "", uint port = 80) {

  WiFiClient WiFi_client;

  JsonDocument RESP = ReadJSON();
  string comm = "Sending " + string(HTTP_METHOD) + " request to " + string(HOST) + " (Wi-Fi): ";
  
  if (!WiFi_client.connect(HOST, port)) {  // check if connection was succesful
    comm.append("\n - Connecting to the server FAILED. ");
    return RESP;
  }

  comm.append("\n - Connecting to the server SUCCESFUL. ");

  // sending request
  WiFi_client.println((String)HTTP_METHOD + " " + URL + " HTTP/1.1");
  WiFi_client.println((String) "Host: " + HOST);
  WiFi_client.println(HEADERS);
  WiFi_client.println("Connection: close");
  WiFi_client.println();
  WiFi_client.println(BODY);

  // wait for response MESSAGE to be available
  unsigned long timeout = millis();
  while (!WiFi_client.available()) {
    if (millis() - timeout > 5000) {

      comm.append("\n - Client TIMEOUT.");
      
      WiFi_client.stop();
      
      return RESP;
    }
  }

  // read the whole response and save it into string
  string response_text = "";
  while (WiFi_client.available()) {
    char c = WiFi_client.read();
    response_text.push_back(c);
  }

  WiFi_client.stop();  // stop connection

  int err = FindResponseCode(response_text, comm);  // check response code
  RESP = ReadJSON(response_text);

  if (err == 200)
    comm = comm + "\n - Request SUCCESFUL (Response code: " + std::to_string(err) + ").";

  else
    comm = comm + "\n - Request FAILED (Response code: " + std::to_string(err) + ").";

  if (WiFi.status() != WL_CONNECTED)
    CONNECTED = false;

  RESP["comm"] = comm;
  RESP["err"] = err;
  
  return RESP;  // return empty string if request failed
}

#endif