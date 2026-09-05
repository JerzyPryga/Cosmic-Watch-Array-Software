#include <string>
#ifndef CW_R_FUNCTIONS_HH
#define CW_R_FUNCTIONS_HH

#include "CW_local_functions.h"

/*-----------NTP request--------------*/
void EthernetSendNTP(const char *address)  // send an NTP request to the time server at the given address
{
  // set all bytes in the buffer to 0
  memset(packet_buffer, 0, NTP_pocket_size);
  // Initialize values needed to form NTP request
  packet_buffer[0] = 0b11100011;  // LI, Version, Mode
  packet_buffer[1] = 0;           // Stratum, or type of clock
  packet_buffer[2] = 6;           // Polling Interval
  packet_buffer[3] = 0xEC;        // Peer Clock Precision
                                  // 8 bytes of zero for Root Delay & Root Dispersion
  packet_buffer[12] = 49;
  packet_buffer[13] = 0x4E;
  packet_buffer[14] = 49;
  packet_buffer[15] = 52;

  ethernet_UDP.beginPacket(address, 123);  //NTP requests are to port 123
  ethernet_UDP.write(packet_buffer, NTP_pocket_size);
  ethernet_UDP.endPacket();
}

/*-------- NTP code ----------*/
time_t EthernetNTPtime() {

  while (ethernet_UDP.parsePacket() > 0)
    ;  // discard any previously received packets

  Serial.println("Transmit NTP Request (Ethernet): ");
  EthernetSendNTP(time_server);

  uint32_t begin_wait = millis();
  while (millis() - begin_wait < 2000) {

    int size = ethernet_UDP.parsePacket();
    if (size >= NTP_pocket_size) {

      Serial.println(" - NTP response RECEIVED. ");
      ethernet_UDP.read(packet_buffer, NTP_pocket_size);  // read packet into the buffer
      AGE_START = micros();

      unsigned long secs_since_1900;  // convert four bytes starting at location 40 to a long integer
      secs_since_1900 = (unsigned long)packet_buffer[40] << 24;
      secs_since_1900 |= (unsigned long)packet_buffer[41] << 16;
      secs_since_1900 |= (unsigned long)packet_buffer[42] << 8;
      secs_since_1900 |= (unsigned long)packet_buffer[43];

      return secs_since_1900 - 2208988800UL;
    }
  }
  Serial.println(" - NTP response NOT RECEIVED. ");
  return 0;  // return 0 if unable to get the time
}

/*----------Attempt Internet conncetion via ethernet-------------*/
int EthernetTry() {

  Serial.print(" - ETHERNET: ");

  teensyMAC(mac);

  qindesign::network::Ethernet.end();

  qindesign::network::Ethernet.init(13);
  int is_begin = qindesign::network::Ethernet.begin(mac, 5000);
  if (is_begin) {
    Serial.println("SUCCCESFULLY connected.");

    ethernet_OK = true;
    return 1;
  }

  ethernet_OK = false;

  if (qindesign::network::Ethernet.linkStatus() != 1)
    Serial.println("FAILED to configure Ethernet. Ethernet cable not connected.");

  else
    Serial.println("FAILED to configure Ethernet. ");

  return 0;
}

/*----------Attempt Internet conncetion via ethernet-------------*/
int WiFiTry() {
  Serial.print(" - WiFi: ");

  if (!ESPcommCHECK())
    Serial.print("Potential PROBLEM with communication with ESP32 board. ");
  else
    Serial.print("Communication with ESP32 board SUCCESSFUL. ");

  JSON_message.clear();
  JSON_message["command"] = "CONNECT";
  JSON_message["WiFi_ID"] = WiFi_ID;
  JSON_message["WiFi_password"] = WiFi_password;

  serializeJson(JSON_message, ESPcomm);
  ESPcomm.flush();

  if (!ReadESP()) {
    Serial.println("FAILED to configure Wi-Fi. No response from ESP32 board.");
    return 0;
  }

  Serial.println((String)JSON_response["comm"]);

  WiFi_OK = JSON_response["err"];

  return WiFi_OK;
}

/*----------Attempt time synchronisation-------------*/
int TimeSyncTry() {

  if (WiFi_OK) {  // synchronize time through NTP with Wi-Fi
    JSON_message["command"] = "NTP";

    Serial.println("Transmit NTP Request (Wi-Fi): ");

    serializeJson(JSON_message, ESPcomm);
    ESPcomm.flush();

    AGE_START = micros();
    if (ReadESP() && JSON_response["err"]) {

      int n_s = ((micros() - AGE_START) + JSON_response["age"].as<int>()) / 1000000;
      delayMicroseconds(((n_s + 1) * 1000000) - (((micros() - AGE_START) + JSON_response["age"].as<int>()) % 1000000));  //correction

      setTime(JSON_response["time_t"].as<uint32_t>() + n_s + 1);

      Serial.println(" - NTP response RECEIVED. ");
      provider = "NTP&IP";
      return 1;
    }

    Serial.println(" - NTP response NOT RECEIVED. ");
  }

  if (ethernet_OK) {  // synchronize time through NTP with Ethernet
    ethernet_UDP.begin(local_port);

    if (EthernetNTPtime()) {

      int n_s = (micros() - AGE_START) / 1000000;
      delayMicroseconds((n_s + 1) * 1000000 - ((micros() - AGE_START) % 1000000));  //correction

      setTime(EthernetNTPtime() + n_s + 1);
      ethernet_UDP.stop();

      provider = "NTP&IP";
      return 1;
    }
    ethernet_UDP.stop();
  }

  Serial.println(" - Get internal RTC time.");
  setSyncProvider(getTeensy3Time);
  setSyncInterval(time_sync_interval);

  return 0;
}

/*----------Find RESPONSE code-------------*/
int FindResponseCode(string resp_str) {

  string str = "0";
  if (resp_str.find("HTTP/1.1 ") < resp_str.length() && resp_str.find("HTTP/1.1 ") >= 0) {

    resp_str = resp_str.substr(resp_str.find("HTTP/1.1 ") + 9);
    if (resp_str.find(" ") < resp_str.find("/n"))
      str = resp_str.substr(0, resp_str.find(" "));
    else
      str = resp_str.substr(0, resp_str.find("\n"));
  }

  else
    Serial.print("No response code found. ");

  return std::stoi(str);
}

/*----------Extract JSON body from HTTP response-------------*/
JsonDocument ReadJSON(string RESP_str = "") {

  JsonDocument JSON;
  deserializeJson(JSON, "{\"content\":0}");

  while (RESP_str.find("\n") < RESP_str.length() && RESP_str.find("\n") >= 0) {
    RESP_str = RESP_str.substr(RESP_str.find("\n") + 1);

    deserializeJson(JSON, RESP_str);
  }

  return JSON;
}

/*----------Universal ethernet HTTP request-------------*/
JsonDocument EthernetHTTPrequest(const char *HTTP_METHOD, const char *HOST, const char *URL = "", const char *HEADERS = "", const char *BODY = "", uint port = 80) {

  qindesign::network::EthernetClient Ethernet_client;

  JsonDocument RESP = ReadJSON();
  string comm = "Sending " + string(HTTP_METHOD) + " request to " + string(HOST) + " (Ethernet): ";

  if (!Ethernet_client.connect(HOST, port)) {  // check if connection was successful
    comm.append("\n - Connecting to the server FAILED. ");
    return RESP;
  }

  comm.append("\n - Connecting to the server SUCCESSFUL. ");

  // sending request
  Ethernet_client.println((String)HTTP_METHOD + " " + URL + " HTTP/1.1");
  Ethernet_client.println((String) "Host: " + HOST);
  Ethernet_client.println(HEADERS);
  Ethernet_client.println("Connection: close");
  Ethernet_client.println();
  Ethernet_client.println(BODY);

  // wait for response message to be available
  unsigned long timeout = millis();
  while (!Ethernet_client.available()) {
    if (millis() - timeout > 5000) {

      comm.append("\n - Client TIMEOUT.");

      Ethernet_client.stop();

      return RESP;
    }
  }

  // read the whole response and save it into string
  string response_text = "";
  while (Ethernet_client.available()) {
    char c = Ethernet_client.read();
    response_text.push_back(c);
  }

  Ethernet_client.stop();  // stop connection

  int err = FindResponseCode(response_text);  // check response code
  RESP = ReadJSON(response_text);

  if (err == 200)
    comm = comm + "\n - Request SUCCESSFUL (Response code: " + std::to_string(err) + ").";

  else {
    comm = comm + "\n - Request FAILED (Response code: " + std::to_string(err) + ").";

    if (qindesign::network::Ethernet.linkStatus() != 1) {
      Serial.println("FAILED to configure Ethernet. Ethernet cable not connected.");
      ethernet_OK = false;
    }
  }

  RESP["comm"] = comm;
  RESP["err"] = err;

  return RESP;  // return empty string if request failed
}

/*----------Geolocalization request-------------*/
int GeolocRequest() {

  string URL = string(geoloc_url) + "apiKey=" + string(geoloc_key);

  if (WiFi_OK) {
    JSON_message["command"] = "REQUEST";
    JSON_message["method"] = "GET";
    JSON_message["host"] = geoloc_host;
    JSON_message["url"] = URL;
    JSON_message["headers"] = "";
    JSON_message["body"] = "";
    JSON_message["port"] = 80;

    serializeJson(JSON_message, ESPcomm);
    ESPcomm.flush();

    ReadESP();
  }

  if (ethernet_OK && JSON_response["err"] != 200)
    JSON_response = EthernetHTTPrequest("GET", geoloc_host, URL.c_str());

  Serial.println((String)JSON_response["comm"]);

  if (JSON_response["err"] == 200) {
    time_zone = JSON_response["offset"].as<double>();
    latitude = JSON_response["latitude"].as<string>();
    longitude = JSON_response["longitude"].as<string>();

    return 1;
  }

  return 0;
}

/*----------Elevation request-------------*/
int ElevationRequest() {

  string URL = string(elevation_url) + "latitude=" + latitude + "&longitude=" + longitude;

  if (WiFi_OK) {
    JSON_message["command"] = "REQUEST";
    JSON_message["method"] = "GET";
    JSON_message["host"] = elevation_host;
    JSON_message["url"] = URL;
    JSON_message["headers"] = "";
    JSON_message["body"] = "";
    JSON_message["port"] = 80;

    serializeJson(JSON_message, ESPcomm);
    ESPcomm.flush();

    ReadESP();
  }

  if (ethernet_OK && JSON_response["err"] != 200)
    JSON_response = EthernetHTTPrequest("GET", elevation_host, URL.c_str());

  Serial.println((String)JSON_response["comm"]);

  if (JSON_response["err"] == 200) {
    altitude = JSON_response["elevation"].as<string>();

    return 1;
  }

  return 0;
}

/*----------Login to CREDO.API database system-------------*/
int LoginRequest() {
  Serial.println("LOGIN ATTEMPT: ");

  JsonDocument BODY_JSON = IntroRequestJSON();
  BODY_JSON["username"] = user_login;
  BODY_JSON["password"] = user_password;

  string BODY_STR;
  serializeJson(BODY_JSON, BODY_STR);

  string HEADERS = "Content-Type: application/json\r\n\
Content-Length: " + std::to_string(BODY_STR.length());

  if (WiFi_OK) {
    JSON_message["command"] = "REQUEST";
    JSON_message["method"] = "POST";
    JSON_message["host"] = credo_host;
    JSON_message["url"] = credo_login_url;
    JSON_message["headers"] = HEADERS;
    JSON_message["body"] = BODY_STR;
    JSON_message["port"] = 443;

    serializeJson(JSON_message, ESPcomm);
    ESPcomm.flush();

    ReadESP();
  }

  if (ethernet_OK && JSON_response["err"] != 200)
    JSON_response = EthernetHTTPrequest("POST", credo_host, credo_login_url, HEADERS.c_str(), BODY_STR.c_str(), 443);
  Serial.println((String)JSON_response["comm"]);

  if (JSON_response["err"] == 200) {
    user_token = JSON_response["token"].as<string>();
    Serial.println(" - USER TOKEN RECEIVED.");

    CONNECTED = true;
    return 1;
  }

  CONNECTED = false;
  return 0;
}

/*----------Sending PING to CREDO.API database-------------*/
int PingRequest() {
  Serial.println("SENDING PING: ");
  uint64_t ts_temp = TimestampRead();

  string timestamp = std::to_string(ts_temp);
  string delta_time = std::to_string(ts_temp - timestamp_last);
  string on_time = std::to_string(ts_temp - time_start);

  if (timestamp.length() > 6) {
    // timestamp = timestamp.erase(timestamp.length() - 4) + "0"; // in [us]
    // delta_time = delta_time.erase(delta_time.length() - 4) + "0"; // in [us]
    // on_time = on_time.erase(on_time.length() - 4) + "0"; // in [us]
    timestamp = timestamp.erase(timestamp.length() - 6);     // in [ms]
    delta_time = delta_time.erase(delta_time.length() - 6);  // in [ms]
    on_time = on_time.erase(on_time.length() - 6);           // in [ms]
  }

  JsonDocument BODY_JSON = IntroRequestJSON();
  BODY_JSON["delta_time"] = delta_time;
  BODY_JSON["on_time"] = on_time;
  BODY_JSON["timestamp"] = timestamp;

  string BODY_STR;
  serializeJson(BODY_JSON, BODY_STR);

  string HEADERS = "Content-Type: application/json\r\n\
Content-Length: " + std::to_string(BODY_STR.length())
                   + "\r\n\
Authorization: Token "
                   + user_token;

  if (WiFi_OK) {
    JSON_message["command"] = "REQUEST";
    JSON_message["method"] = "POST";
    JSON_message["host"] = credo_host;
    JSON_message["url"] = credo_ping_url;
    JSON_message["headers"] = HEADERS;
    JSON_message["body"] = BODY_STR;
    JSON_message["port"] = 443;

    serializeJson(JSON_message, ESPcomm);
    ESPcomm.flush();

    ReadESP();
  }

  if (ethernet_OK && JSON_response["err"] != 200)
    JSON_response = EthernetHTTPrequest("POST", credo_host, credo_ping_url, HEADERS.c_str(), BODY_STR.c_str(), 443);

  Serial.println((String)JSON_response["comm"]);

  if (JSON_response["err"] == 200) {
    CONNECTED = true;
    return 1;
  }

  CONNECTED = false;
  return 0;
}

/*----------Send DETECTIONS request-------------*/
int DataRequest(JsonDocument CONTENT_JSON) {

  JsonDocument BODY_JSON = IntroRequestJSON();
  BODY_JSON["detections"] = CONTENT_JSON;

  string BODY_STR;
  serializeJson(BODY_JSON, BODY_STR);

  string HEADERS = "Content-Type: application/json\r\n\
Content-Length: " + std::to_string(BODY_STR.length())
                   + "\r\n\
Authorization: Token "
                   + user_token;

  if (WiFi_OK) {
    JSON_message["command"] = "REQUEST";
    JSON_message["method"] = "POST";
    JSON_message["host"] = credo_host;
    JSON_message["url"] = credo_dets_url;
    JSON_message["headers"] = HEADERS;
    JSON_message["body"] = BODY_STR;
    JSON_message["port"] = 443;

    serializeJson(JSON_message, ESPcomm);
    ESPcomm.flush();

    ReadESP();
  }

  if (ethernet_OK && JSON_response["err"] != 200)
    JSON_response = EthernetHTTPrequest("POST", credo_host, credo_dets_url, HEADERS.c_str(), BODY_STR.c_str(), 443);

  Serial.println((String)JSON_response["comm"]);

  if (JSON_response["err"] == 200) {
    CONNECTED = true;
    return 1;
  }

  CONNECTED = false;
  return 0;
}

/*----------Send gathered data (in packages)-------------*/
int DATAsend() {

  Serial.print("SENDING DATA: ");

  File file = SD.open(detections_temp_file, FILE_READ);
  if (file) {

    string content = "";
    JsonDocument CONTENT_JSON;

    Serial.println(("Reading data from " + string(detections_temp_file) + " file.").c_str());

    char c_temp;
    while (file.available()) {
      c_temp = file.read();
      if (c_temp == '@' && CONNECTED) {

        deserializeJson(CONTENT_JSON, content);

        if (DataRequest(CONTENT_JSON)) {
          content.clear();
          continue;
        }

        else {
          Serial.println(" - PROBLEM during data transmission.");
          return 0;
        }

      }

      else
        content.push_back(c_temp);
    }

    SD.remove(detections_temp_file);

    return 1;
  }

  Serial.println(("Could not open " + string(detections_temp_file) + " file.").c_str());

  return 0;
}

/*----------Try to connect to the Internet and login to CREDO.API-------------*/
int ConnectTry() {

  Serial.println("Internet connection attempt:");

  EthernetTry();
  WiFiTry();

  if (ethernet_OK || WiFi_OK) {

    user_token = "";
    if (LoginRequest())  //Login to CREDO.API database to get user token
      return 1;
  }

  return 0;
}

#endif
