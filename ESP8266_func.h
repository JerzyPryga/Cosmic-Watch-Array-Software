#ifndef ESP_FUNCTIONS_HH
#define ESP_FUNCTIONS_HH

#include "ESP8266_lib.h"

/*----------Attempt Internet conncetion via WiFi-------------*/
int WiFiTry() {
  Serial.print(" - WiFi: ");

  WiFi.disconnect();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFi_NAME.c_str(), WiFi_password.c_str());

  int wait = 0, d_wait = 100; // [ms]
  while (WiFi.status() != WL_CONNECTED && wait < 5000) {
    delay(d_wait);
    Serial.print(".");
    wait = wait + d_wait;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("CONNECTED.");
    Serial.print(" - Local IP: ");
    Serial.println(WiFi.localIP());

    return 1;
  }

  return 0;
}

/*----------Find RESPONSE code-------------*/
int FindResponseCode(string RESP_str) {

  string str = "0";
  if (RESP_str.find("HTTP/1.1 ") < RESP_str.length() && RESP_str.find("HTTP/1.1 ") >= 0) {

    RESP_str = RESP_str.substr(RESP_str.find("HTTP/1.1 ") + 9);
    if (RESP_str.find(" ") < RESP_str.find("/n"))
      str = RESP_str.substr(0, RESP_str.find(" "));
    else
      str = RESP_str.substr(0, RESP_str.find("\n"));
  }

  else
    Serial.println(" - No response code found.");

  return std::stoi(str);
}

/*----------Find value in JSON-------------*/
string FindInJSON(string JSON_str, const string VAR) {

  string str = "";
  if (JSON_str.find("\"" + VAR + "\":") < JSON_str.length() && JSON_str.find("\"" + VAR + "\":") >= 0) {

    JSON_str = JSON_str.substr(JSON_str.find("\"" + VAR + "\":") + VAR.length() + 3);

    if (JSON_str[0] == '\"') {
      JSON_str = JSON_str.substr(1);
      str = JSON_str.substr(0, JSON_str.find("\""));
    }

    if (JSON_str[0] == '[') {
      JSON_str = JSON_str.substr(1);
      str = JSON_str.substr(0, JSON_str.find("]"));
    }

    else
      str = JSON_str.substr(0, JSON_str.find(","));

    Serial.println((String) " - " + VAR.c_str() + " found: " + str.c_str());
  }

  else
    Serial.println((String) " - No " + VAR.c_str() + " found in the response.");

  return str;
}

/*----------Universal ethernet HTTP request-------------*/
string WiFi_HTTP_request(const char *HTTP_METHOD, const char *HOST, const char *URL = "", const char *HEADERS = "", const char *BODY = "", uint port = 80) {

  Serial.println((String) "Sending " + HTTP_METHOD + " request to " + HOST + ": ");

  if (!WiFi_client.connect(HOST, port)) { // check if connection was succesful
    Serial.println((String) " - Connecting to the server FAILED. ");
    return "";
  }

  Serial.println(" - Connecting to the server SUCCESFUL. ");

  WiFi_client.println((String)HTTP_METHOD + " " + URL + " HTTP/1.1");
  WiFi_client.println((String) "Host: " + HOST);
  if (string(HEADERS) != "")
    WiFi_client.println(HEADERS);
  WiFi_client.println("Connection: close");
  WiFi_client.println();

  if (string(BODY) != "")
    WiFi_client.println(BODY);

  // wait for data to be available
  unsigned long timeout = millis();
  while (!WiFi_client.available()) {
    if (millis() - timeout > 5000) {
      Serial.println(" - Client TIMEOUT.");
      WiFi_client.stop();
      return "";
    }
    yield();
  }

  // read the whole response and save it into string
  string response_text = "";
  while(WiFi_client.available()) {
    char c = WiFi_client.read();
    response_text.push_back(c);
    yield();
  }

  WiFi_client.stop(); // stop connection

  // Serial.print(response_text.c_str());
  int err = FindResponseCode(response_text);  // check response code
  if (err == 200) {
    Serial.println((String) " - Request SUCCESFUL (Response code: " + err + ").");
    return response_text;
  }

  else
    Serial.println((String) " - Request FAILED (Response code: " + err + ").");

  return "";
}

/*----------Geolocalization request-------------*/
void GeolocRequest() {

  string URL = string(GEOLOC_URL) + "apiKey=" + string(GEOLOC_KEY);
  string GEOAPI_RESPONSE = WiFi_HTTP_request("GET", GEOLOC_HOST, URL.c_str());

  if (GEOAPI_RESPONSE.length() > 0) {
    time_zone = std::stod(FindInJSON(GEOAPI_RESPONSE, "offset"));
    latitude = FindInJSON(GEOAPI_RESPONSE, "latitude");
    longitude = FindInJSON(GEOAPI_RESPONSE, "longitude");
  }
}

/*----------Elevation request-------------*/
void ElevationRequest() {

  string URL = string(ELEVATION_URL) + "latitude=" + latitude + "&longitude=" + longitude;
  string GEO_TOPO_RESPONSE = WiFi_HTTP_request("GET", ELEVATION_HOST, URL.c_str());

  if (GEO_TOPO_RESPONSE.length() > 0) {
    altitude = FindInJSON(GEO_TOPO_RESPONSE, "elevation");
  }
}

/*----------Create begining of data_send json string-------------*/
string IntroRequestJSON() {

  string data_send = "{\n\
\"device_id\":\"" + device_ID
                     + "\",\n\
\"device_type\":\"" + device_type
                     + "\",\n\
\"device_model\":\"" + device_model
                     + "\",\n\
\"system_version\":\""
                     + system_version
                     + "\",\n\
\"app_version\":\"" + app_version
                     + "\",";

  return data_send;
}

/*----------Login to CREDO.API database system-------------*/
int LOGIN_REQUEST() {

  string BODY = IntroRequestJSON();
  BODY.append("\n\
\"username\":\""
              + user_login
              + "\",\n\
\"password\":\""
              + user_password
              + "\"\n\
}");
  string HEADERS = "Content-Type: application/json\r\n\
Content-Length: " + std::to_string(BODY.length());

  string CREDO_LOGIN_RESPONSE = WiFi_HTTP_request("POST", CREDO_HOST, CREDO_LOGIN_URL, HEADERS.c_str(), BODY.c_str(), 443);

  if (CREDO_LOGIN_RESPONSE.length() > 0) {
    user_token = FindInJSON(CREDO_LOGIN_RESPONSE, "token");
    CONNECTED = true;
    return 1;
  }

  CONNECTED = false;
  return 0;
}

/*----------Try to connect to the Internet-------------*/
int ConnectTry() {

  Serial.println("\nInternet connection attempt:");

  if (!WiFiTry())
    return 0;

  else {
    GeolocRequest();     //sending request for Geolocalization to obtain from IP
    ElevationRequest();  //sending request for altitude to obtain from coordinates

    user_token = "";

    if (LoginCREDO_API())  //Login to CREDO.API database to get user token
      return 1;
  }

  return 0;
}


/*---------------------Communication with Teensy-------------------*/
string Teensy_Read() {

  string COMMAND = "";

  if (Serial.available()) {
    while (Serial.available()) {
      COMMAND.push_back((char)Serial.read());

      unsigned long timeout = micros();
      while (micros() - timeout < 100) {
        if (Serial.available()) 
          break;
      }
    }
  }

  return COMMAND;
}
#endif
