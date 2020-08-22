/*
 R. J. Tidey 2019/12/30
 Example of using BaseSupport library
 */
 
#include "BaseConfig.h"
#define SWITCH1 12
#define SWITCH2 13
int parameter1, parameter2;
int switch1, switch2;
 
//Put any early set up here like GPIO set up
void setupStart() {
	pinMode(SWITCH1, INPUT_PULLUP);
	pinMode(SWITCH2, INPUT_PULLUP);
}

//load parameters from a config file if required 
void loadConfig() {
	String line = "";
	int config = 0;
	File f = FILESYS.open(CONFIG_FILE, "r");
	if(f) {
		while(f.available()) {
			line =f.readStringUntil('\n');
			line.replace("\r","");
			if(line.length() > 0 && line.charAt(0) != '#') {
				switch(config) {
					case 0: parameter1 = line.toInt(); break;
					case 1: parameter2 = line.toInt();
						Serial.println(F("Config loaded from file OK"));
						break;
				}
				config++;
			}
		}
		f.close();
		Serial.print(F("parameter1:"));Serial.println(parameter1);
		Serial.print(F("parameter2:"));Serial.println(parameter2);
	} else {
		Serial.println(String(CONFIG_FILE) + " not found");
	}
}

void handleStatus() {
	String status = "Switch1: " + String(switch1) + "<BR>";
	status += "Switch2: " + String(switch2);
	server.send(200, "text/html", status);
}

 
// add any extra webServer handlers here
void extraHandlers() {
	server.on("/status", handleStatus);
}

//add any final extra initialisation here 
void setupEnd() {
}

void loop() {
	switch1 = digitalRead(SWITCH1);
	switch2 = digitalRead(SWITCH2);
	server.handleClient();
	wifiConnect(1);
	delaymSec(10);
}

 
