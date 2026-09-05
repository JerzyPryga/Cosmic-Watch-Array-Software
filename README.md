A repository with the software of the detector array based on Cosmic Watches connected in a coincidence cirtcuit.

Files for Teensy4.1 microcrocontroller:
1. TeensyMain.ino - main loop program.
2. TeensyLib.h - library header file.
3. TeensyRemoteFunc.h - definitions of functions used to connect to the internet and interact with the Wi_fi module.
4. TeensyLocalFunc.h - definitions of local functions.

Files for Wi-fi module (ESP32 microcontroller):
1. ESP32.ino - main loop file.
2. ESP32_lib.h - library header file.
3. ESP32_func.h - definitions of some functions.

Files for GPS module (ESP8266 microcontroller):
1. ESP8266.ino - main loop file.
2. ESP8266_lib.h - library header file.
3. ESP8266_func.h - definitions of some functions.

Files for comunication with the server:
1. detection.json - exemplary data file.
2. login.json - a file with login request.
3. ping.json - a file with ping sent to maintain communication with the server.
