/*
 R. J. Tidey 2019/12/30
 Basic support include
 A common core of functions often used in applications
 WifiManager
 WebServer
 UpdateServer
 Spiffs or LittleFS
 FileBrowsing
 */
#define ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

#ifndef FILESYSTYPE
#define FILESYSTYPE 0
#endif

#ifndef WM_PORTALTIMEOUT
#define WM_PORTALTIMEOUT 180
#endif

#if FILESYSTYPE == 0
	#include "LittleFS.h"
	#define FILESYS LittleFS
#else
	#include "FS.h"
	#define FILESYS SPIFFS
#endif

int setupWifi = 1;
/*
Wifi Manager Web set up
*/
#ifdef WM_NAME
	WiFiManager wifiManager;
#endif
char wmName[33];

#define WIFI_CHECK_TIMEOUT 30000
unsigned long wifiCheckTime;

//holds the current upload
File fsUploadFile;

//For update service
const char* update_path = "/firmware";
const char* update_username = "admin";

//AP definitions
#define AP_MAX_WAIT 10
#ifndef AP_PORT
	#define AP_PORT 80
#endif
String macAddr;

ESP8266WebServer server(AP_PORT);
ESP8266HTTPUpdateServer httpUpdater;

void ICACHE_RAM_ATTR  delaymSec(unsigned long mSec) {
	unsigned long ms = mSec;
	while(ms > 100) {
		delay(100);
		ms -= 100;
		ESP.wdtFeed();
	}
	delay(ms);
	ESP.wdtFeed();
	yield();
}

void ICACHE_RAM_ATTR  delayuSec(unsigned long uSec) {
	unsigned long us = uSec;
	while(us > 100000) {
		delay(100);
		us -= 100000;
		ESP.wdtFeed();
	}
	delayMicroseconds(us);
	ESP.wdtFeed();
	yield();
}

void defaultIO() {
	pinMode(0,INPUT_PULLUP);
	pinMode(2,INPUT_PULLUP);
	pinMode(4,INPUT_PULLUP);
	pinMode(5,INPUT_PULLUP);
	pinMode(12,INPUT_PULLUP);
	pinMode(13,INPUT_PULLUP);
	pinMode(14,INPUT_PULLUP);
	pinMode(15,INPUT_PULLUP);
	pinMode(16,INPUT_PULLDOWN_16);
}

/*
  Connect to local wifi with retries
  If check is set then test the connection and re-establish if timed out
*/
int wifiConnect(int check) {
	unsigned long m = millis();
	if(check) {
		if((m - wifiCheckTime) > WIFI_CHECK_TIMEOUT) {
			if(WiFi.status() != WL_CONNECTED) {
				Serial.println(F("Wifi connection timed out. Try to relink"));
			} else {
				wifiCheckTime = m;
				return 1;
			}
		} else {
			return 1;
		}
	}
	wifiCheckTime = m;
	Serial.println(F("Set up managed Web"));
#ifdef WM_STATIC_IP
	wifiManager.setSTAStaticIPConfig(IPAddress(WM_STATIC_IP), IPAddress(WM_STATIC_GATEWAY), IPAddress(255,255,255,0));
#endif
#ifdef FASTCONNECT
	wifiManager.setFastConnectMode(FASTCONNECT);
#endif
	wifiManager.setConfigPortalTimeout(WM_PORTALTIMEOUT);
	//Revert to STA if wifimanager times out as otherwise APA is left on.
	strcpy(wmName, WM_NAME);
	strcat(wmName, macAddr.c_str());
	wifiManager.autoConnect(wmName, WM_PASSWORD);
	WiFi.mode(WIFI_STA);
}

void initFS() {
	if(!FILESYS.begin()) {
		Serial.println(F("No SIFFS found. Format it"));
		if(FILESYS.format()) {
			FILESYS.begin();
		} else {
			Serial.println(F("No SIFFS found. Format it"));
		}
	} else {
		Serial.println(F("FILESYS file list"));
		Dir dir = FILESYS.openDir("/");
		while (dir.next()) {
			Serial.print(dir.fileName());
			Serial.print(F(" - "));
			Serial.println(dir.fileSize());
		}
	}
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  Serial.printf_P(PSTR("handleFileRead: %s\r\n"), path.c_str());
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(FILESYS.exists(pathWithGz) || FILESYS.exists(path)){
    if(FILESYS.exists(pathWithGz))
      path += ".gz";
    File file = FILESYS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.printf_P(PSTR("handleFileUpload Name: %s\r\n"), filename.c_str());
    fsUploadFile = FILESYS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    Serial.printf_P(PSTR("handleFileUpload Data: %d\r\n"), upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    Serial.printf_P(PSTR("handleFileUpload Size: %d\r\n"), upload.totalSize);
  }
}

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.printf_P(PSTR("handleFileDelete: %s\r\n"),path.c_str());
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!FILESYS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  FILESYS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.printf_P(PSTR("handleFileCreate: %s\r\n"),path.c_str());
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(FILESYS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = FILESYS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  Serial.printf_P(PSTR("handleFileList: %s\r\n"),path.c_str());
  Dir dir = FILESYS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  output += "]";
  server.send(200, "text/json", output);
}

void handleMinimalUpload() {
  char temp[700];

  snprintf ( temp, 700,
    "<!DOCTYPE html>\
    <html>\
      <head>\
        <title>ESP8266 Upload</title>\
        <meta charset=\"utf-8\">\
        <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
      </head>\
      <body>\
        <form action=\"/edit\" method=\"post\" enctype=\"multipart/form-data\">\
          <input type=\"file\" name=\"data\">\
          <input type=\"text\" name=\"path\" value=\"/\">\
          <button>Upload</button>\
         </form>\
      </body>\
    </html>"
  );
  server.send ( 200, "text/html", temp );
}

void handleFileSysFormat() {
	FILESYS.format();
	server.send(200, "text/json", "format complete");
}

/*
  Set up
*/
extern void setupStart();
extern void loadConfig();
extern void extraHandlers();
extern void setupEnd();

void setup() {
	defaultIO();
	Serial.begin(115200);
#ifdef SETUP_START
	setupStart();
#endif
#ifdef SETUP_FILESYS
	Serial.println(F("Set up filing system"));
	initFS();
#endif
	macAddr = WiFi.macAddress();
	macAddr.replace(":","");
	Serial.println(macAddr);

#ifdef CONFIG_FILE
	loadConfig();
#endif
	if(setupWifi) {
		Serial.println(F("Set up Wifi services"));
		wifiConnect(0);
		//Update service
		MDNS.begin(host.c_str());
		httpUpdater.setup(&server, update_path, update_username, update_password);

#ifdef SETUP_SERVER
		Serial.println(F("Set up web server"));
		//Simple upload
		server.on("/upload", handleMinimalUpload);
		server.on("/format", handleFileSysFormat);
		server.on("/list", HTTP_GET, handleFileList);
		//load editor
		server.on("/edit", HTTP_GET, [](){
		if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");});
		//create file
		server.on("/edit", HTTP_PUT, handleFileCreate);
		//delete file
		server.on("/edit", HTTP_DELETE, handleFileDelete);
		//first callback is called after the request has ended with all parsed arguments
		//second callback handles file uploads at that location
		server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);
		//called when the url is not defined here
		//use it to load content from File System
		server.onNotFound([](){if(!handleFileRead(server.uri())) server.send(404, "text/plain", "FileNotFound");});
#ifdef SETUP_START
		extraHandlers();
#endif
		server.begin();
#endif
		MDNS.addService("http", "tcp", 80);
	}
#ifdef SETUP_END
		setupEnd();
#endif
	Serial.println(F("Set up complete"));
}
