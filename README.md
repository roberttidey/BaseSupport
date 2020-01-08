# BaseSupport library
## setup and routines commonly used in applications

### Features
- setup routine with hooks for customisation
- WifiManager
- OTA webserver
- SPIFFS
- WebServer
- Web FS access / editing
- delay routines

### Usage
- install into libraries folder
- put BaseConfig.h (from examples) into the app ino folder
- edit BaseConfig file
	- name / passwords for WifiManager and OTA
	- defines for customisation of setup
- add #include "BaseConfig.h" to top of ino file  

