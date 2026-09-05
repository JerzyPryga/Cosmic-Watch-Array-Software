#ifndef ESP_LIBS_HH
#define ESP_LIBS_HH

#include "Arduino.h"
#include "Esp.h"

#include <TimeLib.h>
#include <SD.h>
#include <Wire.h>
#include <stdio.h>
#include <chrono>
#include <string>
#include <sstream>

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#define TeensyComm Serial1

/*--------------------- Variables and constants to read data -------------------*/
using string = std::string; //alias
string TimeColsPrint = "", TimeColsSave = "";
const int LED_CW_time = 100000, n_CW = 8;  // Teensy on-board SD
int LED_CW_count[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int event = 0, n_event = 1, sum_b = 0, b = 0, Audio_count = 0;
const int buffer_length = 1000, package_size = 5; //package_size has to be <= buffer_length
int DATA_ARRAY[buffer_length];

string MESSAGE = "";
/*--------------------- Variables to measure time -------------------*/
using timePoint = std::chrono::system_clock::time_point;  // alias
//timePoint TIME_ARRAY[buffer_length];
uint64_t TIME_ARRAY[buffer_length];
const double dts = 32.768;
uint64_t timestamp = 0, timestamp_last = 0, time_start = 0;

/*--------------------- Device parameters -------------------*/
int event_ID, user_ID, team_ID;
string latitude = "0", longitude = "0"; //in [deg]
string altitude = "0"; // in [m]
string provider = "IP", source = "CW_0.1", device_ID = "1", device_type = "CWarray", device_model = "m0.5", system_version = "OS.0.5", app_version = "1";
string exact_address = "UKEN";
double time_zone = 2;  // Central European Time

/*--------------------- Database parameters -------------------*/
const char CREDO_HOST[] = "api.credo.science";
const char CREDO_LOGIN_URL[] = "https://api.credo.science/api/v2/user/login";
const char CREDO_PING_URL[] = "https://api.credo.science/api/v2/ping";
const char CREDO_DETS_URL[] = "https://api.credo.science/api/v2/detection";
//double connect_req_time = 3*60; // time between attempts to connect to the server [s]

/*--------------------- User parameters -------------------*/
string user_login = "jerzyPryga", user_password = "Karoljerzy1";
string user_token = "";

string WiFi_NAME = "TP-Link_23E6", WiFi_password = "90168178";

/*--------------------- Variables to read time from time server -------------------*/
const int time_sync = 6 * 3600; //[s]
const int NTP_PACKET_SIZE = 48;      // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE];  //buffer to hold incoming & outgoing packets
// NTP Servers:
const char timeServer[] = "vega.cbk.poznan.pl";  //cesium clock 5071A CBK PAN
//IPAddress timeServer(150, 254, 190, 51);
//const char timeServer[] = "pool.ntp.org";
//const char timeServer[] = "europe.pool.ntp.org";
// const char timeServer[] = "time.nist.gov";

/*--------------------- Variables and constants for internet connection -------------------*/
// WiFiUDP UDP_object;
// qindesign::network::EthernetUDP UDP_object;
byte mac[6];
unsigned int localPort = 8888;  // local port to listen for UDP packets
bool ethernet_OK = false, WiFI_OK = false, CONNECTED = false;

WiFiClient WiFi_client;
//WiFiClientSecure WiFi_SSL_client;
string WiFiname = "TP-Link_23E6", WiFipassword = "90168178";
//string WiFiname = "moto g42_6044", WiFipassword = "12345678";

/*---------------------Variables and constants for gealocalization-------------------*/
const char GEOLOC_HOST[] = "api.ipgeolocation.io";
const char GEOLOC_URL[] = "/ipgeo?";
const char GEOLOC_KEY[] = "f8839ee8cfb6461daca1cc9cfc6e85c3";

const char ELEVATION_HOST[] = "api.open-meteo.com";
const char ELEVATION_URL[] = "/v1/elevation?";
const string accuracy = "90"; // in [m]

#endif
