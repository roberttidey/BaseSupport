# BaseSupport library
## setup and routines commonly used in applications

### Features
- setup routine with hooks for customisation
- WifiManager
- OTA webserver
- SPIFFS or LittleFS
- WebServer
- Web FS access / editing
- delay routines

### Quick Connect support
- Supports fast connect of WiFi (1 second instead of normal 3 seconds)
- Use fork of WiFiManager at https://github.com/roberttidey/WiFiManager
- Use development branch
- Configure by having #define FASTCONNECT true in BaseConfig.h

### Usage
- install into libraries folder
- put BaseConfig.h (from examples) into the app ino folder
- edit BaseConfig file
	- name / passwords for WifiManager and OTA
	- defines for customisation of setup
- add #include "BaseConfig.h" to top of ino file
- after compiling and first serial upload use ip/upload to upload files from data folder
- ip/edit can then be used to access the filing system
- use ip/firmware to do OTA updates of new binaries

### Important
Support for SPIFFS or LittleFS has been added. This affects the BaseConfig file.
Use FILESYS. instead of SPIFFS. or LittleFS. in the app code

