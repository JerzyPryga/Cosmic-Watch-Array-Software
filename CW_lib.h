#ifndef CW_LIBS_HH
#define CW_LIBS_HH

#include "Arduino.h"
#include <TeensyID.h>
#include <TimeLib.h>
#include <TinyGPS.h>
#include <QNEthernet.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <stdio.h>
#include <chrono>
#include <string>
#include <sstream>
#include <Adafruit_MPL115A2.h>
#include <ArduinoJson.h>

//Pins definitions
#define _SPIDEVICE_CS_ 10
#define _COINC_ 20
#define _TRIGGER_ 8
#define _ON_LED_ 23
#define _SERVER_CONN_LED_ 21
#define _SD_LED_ 22
#define _SOUND_ 25
#define _ESP_RESET_ 32
#define _RX3_ 15
#define _TX3_ 14
#define _RX8_ 34
#define _TX8_ 35

#define ESPcomm Serial3 //Serial communication with ESP board
#define GPScomm Serial8 //Serial communication with GPS board

using string = std::string; //alias
using timePoint = std::chrono::system_clock::time_point;  // alias

/*--------------------- Class objects and variables for tempereture sensors -------------------*/
Adafruit_MPL115A2 temp_sens_int;  //Internal barometric pressure sensor
Adafruit_MPL115A2 temp_sens_out;  //External barometric pressure sensor
float pressure_in = 0, temperature_in = 0, pressure_out = 0, temperature_out = 0;

/*--------------------- Variables and constants to read data -------------------*/
string time_cols_print = "", time_cols_save = "";
const int LED_array[] = { _COINC_, 7, 6, 5, 4, 3, 2, 1, 0 };
const int chip_select = BUILTIN_SDCARD, CW_LED_time = 100000, n_CW = 8;  // Teensy on-board SD
int CW_LED_count[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int event = 0, n_event = 1, sum_b = 0, b = 0, audio_count = 0;
const int buffer_length = 1000, package_size = 200; //package_size has to be <= buffer_length (max = around 200)
int DATA_ARRAY[buffer_length];
const char data_tsv[] = "data.tsv", detections_file[] = "detections.json", detections_temp_file[] = "detections_temp.json";

/*--------------------- GPS variables -------------------*/
bool GPS_LOCALIZED = false, GPS_TIME = false;
TinyGPS gps;

/*--------------------- Variables to measure time -------------------*/
uint64_t TIME_ARRAY[buffer_length];
const double dts = 33; //32.768 us
uint64_t timestamp_last = 0, time_start = 0, data_send_interval = 5 * 60 * 1E9; 
int time_sync_interval = 5 * 60; // [s]

/*--------------------- Device parameters -------------------*/
int event_ID, user_ID, team_ID;
string latitude = "0", longitude = "0"; //in [deg]
string altitude = "0"; // in [m]
string provider = "IP", source = "CW_0.1", device_ID = "1", device_type = "CWarray", device_model = "m0.5", system_version = "OS.0.5", app_version = "1";
string exact_address = "UKEN";
double time_zone = 2;  // Central European Time

/*--------------------- Communication with ESP32 board -------------------*/
JsonDocument JSON_message, JSON_response;

/*--------------------- Database parameters -------------------*/
const char credo_host[] = "api.credo.science";
const char credo_login_url[] = "https://api.credo.science/api/v2/user/login";
const char credo_ping_url[] = "https://api.credo.science/api/v2/ping";
const char credo_dets_url[] = "https://api.credo.science/api/v2/detection";

/*--------------------- User parameters -------------------*/
string user_login = "jerzyPryga", user_password = "Karoljerzy1";
string user_token = "";

/*--------------------- Variables to read time from time server -------------------*/
const int time_sync = 6 * 3600; //[s]
const int NTP_pocket_size = 48;      // NTP time is in the first 48 bytes of message
byte packet_buffer[NTP_pocket_size];  //buffer to hold incoming & outgoing packets

unsigned long AGE_START;
// NTP Servers:
const char time_server[] = "vega.cbk.poznan.pl";  //cesium clock 5071A CBK PAN
//IPAddress time_server(150, 254, 190, 51);
//const char time_server[] = "pool.ntp.org";
//const char time_server[] = "europe.pool.ntp.org";
// const char time_server[] = "time.nist.gov";

/*--------------------- Variables and constants for internet connection -------------------*/
qindesign::network::EthernetUDP ethernet_UDP;
byte mac[6];
unsigned int local_port = 8888;  // local port to listen for UDP packets
bool ethernet_OK = false, WiFi_OK = false, CONNECTED = false;

string WiFi_ID = "TP-Link_23E6", WiFi_password = "90168178";

/*---------------------Variables and constants for gealocalization-------------------*/
const char geoloc_host[] = "api.ipgeolocation.io";
const char geoloc_url[] = "/ipgeo?";
const char geoloc_key[] = "f8839ee8cfb6461daca1cc9cfc6e85c3";

const char elevation_host[] = "api.open-meteo.com";
const char elevation_url[] = "/v1/elevation?";
const string accuracy = "50"; // in [m]

#endif


