#ifndef ESP_LIBS_HH
#define ESP_LIBS_HH

#include "Esp.h"
#include <stdio.h>
#include <string>
#include <chrono>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
// #include "esp_timer.h"

#define TeensyComm Serial2

using string = std::string; //alias

/*--------------------- Communication with Teensy4.1 board -------------------*/

JsonDocument MESSAGE, RESPONSE;

/*--------------------- Internet connection -------------------*/
bool CONNECTED = false;

/*--------------------- NTP variables -------------------*/
WiFiUDP ntp_UDP;
NTPClient time_client(ntp_UDP);

// auto age_start = std::chrono::system_clock::now();
unsigned long age_start;

#endif
