#ifndef CW_L_FUNCTIONS_HH
#define CW_L_FUNCTIONS_HH

#include "CW_lib.h"

/*----------SD card - create data file + header - tsv-------------*/
// void FileInitTSV() {

//   if (!SD.exists(data_tsv)) {
//     File file = SD.open(data_tsv, FILE_WRITE);
//     if (file) {
//       file.println(latitude.c_str());
//       file.println(longitude.c_str());
//       file.println(altitude.c_str());
//       file.println(accuracy.c_str());
//       file.println((String)dts);
//       file.println((String)n_CW);
//       file.println((String)time_zone);
//       file.print("Timestamp\tDay\tMonth\tYear\tHour\tMinute\tSecond\tInternal T.(degC)\tInternal P(hPa)\tExternal T.(degC)\tExternal P(hPa)");
//       for (int i = 0; i < n_CW; i++) {
//         file.print((String) "\tInput " + (i + 1));
//       }
//       file.println();
//       Serial.println(" - file data.tsv created SUCCEFULLY.");

//       file.close();
//     }

//     else {
//       Serial.println(" - could not open data.tsv file.");
//     }

//   }

//   else {
//     Serial.println(" - data.tsv file exists.");
//   }
// }

/*----------Light up LEDs and turns on sound ----------*/
void LEDsOn(int data_LED) {
  if (data_LED) {
    digitalWriteFast(LED_array[data_LED], HIGH);
    CW_LED_count[data_LED] = CW_LED_time;
  }
}

/*----------Coincidence signalization ----------*/
void CoincAndSoundOn() {

  if (sum_b > 1) {
    digitalWriteFast(_COINC_, HIGH);
    CW_LED_count[0] = sum_b * CW_LED_time;
  }

  tone(_SOUND_, 1600 - (sum_b * 120));  //more signals in coincidence events = lower sound
  audio_count = CW_LED_time;
}

/*----------Shut down LEDs and sound----------*/
void LEDsAndSoundOff() {

  for (int iCW = 0; iCW <= n_CW; iCW++) {
    if (CW_LED_count[iCW] > 0)
      CW_LED_count[iCW]--;

    else
      digitalWriteFast(LED_array[iCW], LOW);
  }

  if (audio_count > 0)
    audio_count--;

  else
    noTone(_SOUND_);
}

/*----------Shut down all LEDs and sound----------*/
void AllOff() {

  delay(250);

  for (int iCW = 0; iCW <= n_CW; iCW++) {
    CW_LED_count[iCW] = 0;
    digitalWriteFast(LED_array[iCW], LOW);
  }

  noTone(_SOUND_);
}

/*-------------Number of RTC oscillations since epoch--------------------*/
uint64_t GetRTCperiods() {  //get the number of oscillator periods since 1970-01-01
  uint32_t hi1 = SNVS_HPRTCMR, lo1 = SNVS_HPRTCLR;
  while (true) {
    uint32_t hi2 = SNVS_HPRTCMR, lo2 = SNVS_HPRTCLR;
    if (lo1 == lo2 && hi1 == hi2) {
      return (uint64_t)hi2 << 32 | lo2;
    }
    hi1 = hi2;
    lo1 = lo2;
  }
}

/*---------- Files on SD append (or create if do not exists)-------------*/
int SDfileWrite(const char *file_name, const char *file_content = "", int OPT = FILE_WRITE) {

  File file = SD.open(file_name, OPT);
  if (file) {
    Serial.println((String) " - File " + file_name + " opened SUCCESSFULLY.");
    file.print(file_content);

    file.close();

    digitalWrite(_SD_LED_, HIGH);
    return 1;
  }

  Serial.println((String) " - Could not open " + file_name + " file.");

  return 0;
}

/*----------Create begining of data_send json string-------------*/
JsonDocument IntroRequestJSON() {

  JsonDocument INTRO;

  INTRO["device_id"] = device_ID;
  INTRO["device_type"] = device_type;
  INTRO["device_model"] = device_model;
  INTRO["system_version"] = system_version;
  INTRO["app_version"] = app_version;

  return INTRO;
}

/*----------Create body of data_send json string-------------*/
JsonDocument GenerateDataJSON(int i) {

  string timestamp = std::to_string(TIME_ARRAY[i]);

  if (timestamp.length() > 6)
    //timestamp = timestamp.erase(timestamp.length() - 4) + "0"; // in [us]
    timestamp = timestamp.erase(timestamp.length() - 6);  // in [ms]

  JsonDocument DATA;
  DATA["accuracy"] = accuracy;
  DATA["altitude"] = altitude;
  if(altitude == "null")
    DATA["altitude"] = "0";

  DATA["latitude"] = latitude;
  if(latitude == "null")
    DATA["latitude"] = "0";
  
  DATA["longitude"] = longitude;
  if(longitude == "null")
    DATA["longitude"] = "0";

  DATA["provider"] = provider;
  DATA["timestamp"] = timestamp;

  JsonDocument METADATA;
  METADATA["t_out"] = temperature_out;
  METADATA["p_out"] = pressure_out;
  METADATA["t_in"] = temperature_in;
  METADATA["p_in"] = pressure_in;
  METADATA["data_value"] = DATA_ARRAY[i];
  METADATA["n_CW"] = n_CW;
  METADATA["exact_location"] = exact_address;

  DATA["metadata"] = METADATA.as<string>();

  return DATA;
}

int SaveDataJSON() {

  string DATA_STR = "";

  string temp;
  int i = 0;
  while (i < event) {

    DATA_STR.append("[");

    for (int j = 0; j < package_size; j++) {
      serializeJson(GenerateDataJSON(i), temp);

      i++;
      if (i == event) {
        DATA_STR.append(temp);
        break;
      }

      if (j < package_size - 1)
        temp.append(",");

      DATA_STR.append(temp);
    }

    DATA_STR.append("]@");

    if (!SDfileWrite(detections_file, DATA_STR.c_str()) || !SDfileWrite(detections_temp_file, DATA_STR.c_str()))
      return 0;

    DATA_STR.clear();
  }

  return 1;
}

/*-------------Get System Clock Timestamp [ns]--------------------*/
uint64_t TimestampRead() {
  return GetRTCperiods() * (1E9 / 32768);  // system clock uses [ns] as time base
}

/*-------------Get time in human form--------------------*/
string TimeDataRead(uint64_t DATA_TIME) {

  std::chrono::system_clock::time_point TP = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(DATA_TIME));
  time_t rawTime = std::chrono::system_clock::to_time_t(TP);  // convert the std::chrono time_point to a traditional time_t value
  tm t = *gmtime(&rawTime);                                   // caluclate year, month... from the raw time_t value and store them in a tm struct

  string year, month, day, hour, min, sec;

  year = std::to_string(t.tm_year + 1900);

  month = std::to_string(t.tm_mon + 1);
  if (t.tm_mon + 1 < 10)
    month = "0" + std::to_string(t.tm_mon + 1);

  day = std::to_string(t.tm_mday);
  if (t.tm_mday < 10)
    day = "0" + std::to_string(t.tm_mday);

  hour = std::to_string(t.tm_hour + int(time_zone));
  if (t.tm_hour < 10)
    hour = "0" + std::to_string(t.tm_hour);

  min = std::to_string(t.tm_min);
  if (t.tm_min < 10)
    min = "0" + std::to_string(t.tm_min);

  sec = std::to_string(t.tm_sec);
  if (t.tm_sec < 10)
    sec = "0" + std::to_string(t.tm_sec);

  string timestamp = std::to_string(DATA_TIME);
  timestamp = timestamp.erase(timestamp.length() - 4) + "0";
  string us = timestamp.substr(timestamp.length() - 6);

  time_cols_save = day + "\t" + month + "\t" + year + "\t" + hour + "\t" + min + "\t" + sec + "." + us;
  time_cols_print = day + "." + month + "." + year + "\t" + hour + ":" + min + ":" + sec + "." + us;

  return timestamp;
}

/*-------------Get preassure and temperature--------------------*/
void BarometricDataRead() {
  temp_sens_int.getPT(&pressure_in, &temperature_in);
  temp_sens_out.getPT(&pressure_out, &temperature_out);

  pressure_in = pressure_in * 10.;    // in [hPa]
  pressure_out = pressure_out * 10.;  // in [hPa]

  if (pressure_out < 600)
    pressure_out = temperature_out = 0;

  if (pressure_in < 600)
    pressure_in = temperature_in = 0;
}

/*-------------Print preassure and temperature--------------------*/
void BarometricDataPrint() {
  Serial.print("\tT_int: ");
  Serial.print(temperature_in, 1);
  Serial.print(" degC\tP_int: ");
  Serial.print(pressure_in);
  Serial.print(" hPa ");
  Serial.print("\tT_out: ");
  Serial.print(temperature_out, 1);
  Serial.print(" degC\tP_out: ");
  Serial.print(pressure_out);
  Serial.print(" hPa ");
}

/*---------------------SD card - time and data write .tsv -------------------*/
// void SaveDataTSV() {

//   if (SD.exists(data_tsv)) {
//     File file = SD.open(data_tsv, FILE_WRITE);
//     if (file) {
//       for (int i = 0; i < event; i++) {

//         string ts = TimeDataRead(TIME_ARRAY[i]);

//         int DATA_WRITE = DATA_ARRAY[i];
//         file.print((String)ts.c_str() + "\t" + time_cols_save.c_str() + "\t" + temperature_in + "\t" + pressure_in + "\t" + temperature_out + "\t" + pressure_out);
//         for (int i = 0; i < n_CW; i++) {
//           b = DATA_WRITE % 2;
//           DATA_WRITE = DATA_WRITE / 2;
//           file.print((String) "\t" + b);
//         }
//         file.println();
//       }
//       file.close();
//     }
//   }
// }

/*---------------------Time and data print-------------------*/
void DataPrint(int DATA_PRINT, uint64_t DATA_TIME) {

  TimeDataRead(DATA_TIME);

  Serial.print((String) "Event nr: " + (event + 1) + "\t");
  Serial.print(time_cols_print.c_str());
  BarometricDataPrint();

  sum_b = 0;
  for (int i = 0; i < n_CW; i++) {
    b = DATA_PRINT % 2;
    DATA_PRINT = DATA_PRINT / 2;

    Serial.print((String) "\t" + b);

    sum_b = sum_b + b;
    LEDsOn((i + 1) * b);
  }

  Serial.println();
}

/*--------Teensy time ----------*/
time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

/*---------------------Communication with WiFi module-------------------*/
int ReadESP() {

  unsigned long timeout = millis();
  while (millis() - timeout < 5000) {
    if (ESPcomm.available())
      break;
  }

  if (ESPcomm.available()) {
    string str = "";

    while (ESPcomm.available()) {
      str.push_back((char)ESPcomm.read());

      timeout = millis();
      while (millis() - timeout < 50) {
        if (ESPcomm.available())
          break;
      }
    }

    deserializeJson(JSON_response, str.c_str());

    return 1;
  }

  return 0;
}

int WriteESP(const char *content = "") {

  if (string(content) != "") {
    ESPcomm.print(content);
    ESPcomm.flush();
    return 1;
  }

  if (Serial.available()) {
    string str = "";

    while (Serial.available()) {
      str.push_back((char)Serial.read());

      unsigned long timeout = micros();
      while (micros() - timeout < 50) {
        if (Serial.available())
          break;
      }
    }

    JSON_message.clear();
    JSON_message["content"] = str;

    serializeJson(JSON_message, ESPcomm);
    ESPcomm.flush();

    Serial.println();
    serializeJson(JSON_message, Serial);
    Serial.flush();

    return 1;
  }

  return 0;
}

/*---------------------Reseting WiFi module-------------------*/
void ResetESP() {
  digitalWrite(_ESP_RESET_, LOW);
  delay(500);
  digitalWrite(_ESP_RESET_, HIGH);
  delay(2000);
}

/*---------------------Checking communication with WiFi module-------------------*/
int ESPcommCHECK() {

  int ESP_err = 0;
  for (int i = 0; i < 3; i++) {
    if (WriteESP("{test}") && ReadESP())
      ESP_err = 1;

    else
      ResetESP();
  }

  return ESP_err;
}

/*---------------------Communication with GPS module-------------------*/
int GPSreadPosition(bool cold = 0) {

  bool new_data = false;
  Serial.println("Reading position from GPS: ");

  unsigned long max_time = 5000;
  if(cold)
    max_time = 30000;

  unsigned long timeout = millis();  
  while (millis() - timeout < max_time) {
    if (GPScomm.available())
      break;
  }

  while (GPScomm.available()) {
    if (gps.encode(GPScomm.read()))
      new_data = true;

    timeout = millis();
    while (millis() - timeout < 100) {
      if (GPScomm.available())
        break;
    }
  }

  if (new_data) {
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);

    longitude = std::to_string(flon);
    latitude = std::to_string(flat);
    altitude = std::to_string(gps.altitude() / 100);

    Serial.println((" - Localization SUCCESSFUL (LON = " + longitude + ", LAT = " + latitude + ", ALT = " + altitude + " m).").c_str());

    GPS_LOCALIZED = true;
    provider = "GPS";
    return 1;
  }

  Serial.println(" - Localization FAILED. No valid satellite signal. ");
  GPS_LOCALIZED = false;
  return 0;
}

int GPStimeSync(bool cold = 0) {

  Serial.println("Synchronizing time through GPS: ");

  unsigned long max_time = 5000;
  if(cold)
    max_time = 30000;

  unsigned long timeout = millis();  
  while (millis() - timeout < max_time) {
    if (GPScomm.available())
      break;
  }

  while (GPScomm.available()) {,,,,,,,,,,,,,,,,,

    AGE_START = micros();
    if (gps.encode(GPScomm.read())) {  // process gps message
      unsigned long age;
      int Year;
      byte Month, Day, Hour, Minute, Second, Hundredths;
      gps.crack_datetime(&Year, &Month, &Day, &Hour, &Minute, &Second, &Hundredths, &age); //read time values into variables

      if (age < 500) {
        int n_s = (micros() - AGE_START)/1000000;
        delay(1000 - (((10 * Hundredths) + age) % 1000));  //apply correction [ms]
        delayMicroseconds(((n_s + 1)*1000000) - ((micros() - AGE_START) % 1000000));  //apply correction [us]

        setTime(Hour, Minute, Second + 2, Day, Month, Year); //synchronize time

        Serial.println(" - Synchronization SUCCESSFUL.");
        GPS_TIME = true;
        provider = "GPS";
        return 1;
      }
    }

    timeout = millis();
    while (millis() - timeout < 100) { //wait for communication to be back
      if (GPScomm.available())
        break;
    }
  }

  Serial.println(" - Synchronization FAILED.");
  GPS_TIME = false;
  return 0;
}

#endif
