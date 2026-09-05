
//#include "CW_local_functions.h"
#include "CW_remote_functions.h"

using namespace qindesign::network;
using namespace std;
using namespace std::chrono;

//****************************** SETUP **********************************//

void setup() {
  /*-----------------------Setting pins modes---------------------*/

  pinMode(_SERVER_CONN_LED_, OUTPUT);
  digitalWrite(_SERVER_CONN_LED_, LOW);

  pinMode(_TRIGGER_, INPUT);
  digitalWrite(_TRIGGER_, LOW);

  pinMode(_ON_LED_, OUTPUT);
  digitalWrite(_ON_LED_, LOW);

  pinMode(_SOUND_, OUTPUT);
  digitalWrite(_SOUND_, LOW);

  pinMode(_SD_LED_, OUTPUT);
  digitalWrite(_SD_LED_, LOW);

  pinMode(_ESP_RESET_, OUTPUT);
  digitalWrite(_ESP_RESET_, HIGH);

  for (int i = 0; i <= n_CW; i++) {
    pinMode(LED_array[i], OUTPUT);
    digitalWrite(LED_array[i], LOW);
  }

  pinMode(LED_BUILTIN, OUTPUT);

  for (int n = 0; n < 3; n++) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(_ON_LED_, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(_ON_LED_, LOW);
    delay(200);
  }

  digitalWrite(LED_BUILTIN, HIGH);

  /*-----------------------Serial communication begin---------------------*/

  Serial.begin(115200);
  ESPcomm.begin(115200);
  GPScomm.begin(9600);

  Serial.println("\n==================================================");
  Serial.println();

  /*----------------------- Setup of SPI input pins ---------------------*/

  pinMode(_SPIDEVICE_CS_, OUTPUT);

  SPI.setMISO(12);
  SPI.begin();
  digitalWrite(_SPIDEVICE_CS_, HIGH);
  SPI.beginTransaction(SPISettings(15000000, MSBFIRST, SPI_MODE0));  // 15MHz

  /*-----------------------Setup temp + pressure sensors---------------------*/

  Wire.begin();
  temp_sens_int.begin();

  Wire1.begin();
  temp_sens_out.begin(&Wire1);

  BarometricDataRead();  //reading values of P and T

  /*-----------------------GPS geolocalization and time synchronization---------------------*/

  GPS_LOCALIZED = GPSreadPosition(1);
  GPS_TIME = GPStimeSync(1);

  /*-----------------------Ethernet or WiFi connection attempt + eventual NTP request---------------------*/

  CONNECTED = ConnectTry();  // attempt internet connection and login to database

  if (!GPS_LOCALIZED) {
    GeolocRequest();     //sending request for Geolocalization to obtain from IP
    ElevationRequest();  //sending request for altitude to obtain from coordinates
  }

  if (!GPS_TIME)
    TimeSyncTry();  //synchronizing time through WiFi or Ethernet

  digitalWrite(_SERVER_CONN_LED_, CONNECTED);

  /*-----------------------Set timestamp of start------------------*/

  timestamp_last = TimestampRead();  // in [ns]
  time_start = TimestampRead();      // in [ns]

  /*-----------------------SD card------------------*/

  Serial.print("Initializing SD card. ");

  if (!SD.begin(chip_select))
    Serial.println("Card FAILED, or not present.");

  else {
    Serial.println("Card initialized SUCCESSFULLY.");
    SD.remove(detections_temp_file);
    digitalWrite(_SD_LED_, HIGH);
  }

  /*------------------------- ON_OFF LED light -----------------------*/

  digitalWrite(_ON_LED_, HIGH);
}

//******************************* MAIN LOOP *********************************//

void loop() {  // Main loop

  if (digitalReadFast(_TRIGGER_)) {
    TIME_ARRAY[event] = TimestampRead();

    digitalWriteFast(_SPIDEVICE_CS_, LOW);
    DATA_ARRAY[event] = SPI.transfer16(0x0000) / 256;
    digitalWriteFast(_SPIDEVICE_CS_, HIGH);

    DataPrint(DATA_ARRAY[event], TIME_ARRAY[event]);
    event++;

    if (event == buffer_length || (TIME_ARRAY[event - 1] - timestamp_last) >= data_send_interval) {
      AllOff();

      BarometricDataRead();

      SaveDataJSON();

      if (!CONNECTED)              // check if connection to the internet and database was OK at last attempt
        CONNECTED = ConnectTry();  // if not try to reconnect  login and obtain user token

      if (CONNECTED && PingRequest() && DATAsend())  // if succesfully connected try to send all the data
        Serial.println("ALL data sent SUCCESSFULLY.");

      else
        Serial.println("FAILED to send all data. Another attempt will be made later.");


      if (!GPS_LOCALIZED)                   // check if position was successfully obtained from GPS during setup
        GPS_LOCALIZED = GPSreadPosition();  // if not try again

      if (!GPS_LOCALIZED) {  // if still not successful try different source
        GeolocRequest();     //sending request for Geolocalization to obtain from IP
        ElevationRequest();  //sending request for altitude to obtain from coordinates
      }

      GPS_TIME = GPStimeSync();  //synchronizing time through GPS

      if (!GPS_TIME)    // if not successfull try different source
        TimeSyncTry();  //synchronizing time through WiFi or Ethernet

      digitalWriteFast(_SERVER_CONN_LED_, CONNECTED);

      event = 0;
      timestamp_last = TimestampRead();
    }
    CoincAndSoundOn();
  }

  LEDsAndSoundOff();
}
//****************************************************************//