/*
 R. J. Tidey 2019/12/30
 Basic config
*/
 
/*
Wifi Manager Web set up
*/
#define WM_NAME "esp8266_wm"
#define WM_PASSWORD "password"

//Update service set up
String host = "esp8266_host";
const char* update_password = "password";

//define actions during setup
//define any call at start of set up
#define SETUP_START 1
//define config file name if used 
#define CONFIG_FILE "/config.txt"
//set to 1 if SPIFFS used
#define SETUP_SPIFFS 1
//define to set up server and reference any extra handlers required
#define SETUP_SERVER 1
//call any extra setup at end
#define SETUP_END 1
#include "BaseSupport.h"
