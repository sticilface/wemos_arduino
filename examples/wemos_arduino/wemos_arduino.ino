#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WemosArduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>



const char* ssid = "xxxxx";
const char* password = "xxxxx";

AsyncWebServer HTTP(80);
Wemos wemos(HTTP); 


void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  wemos.name(device_name); 


    //  the name of the device given blelow is ignored, and only the device 
    wemos.addDevice( new WemosSwitch("test", [](bool state) { 
        Serial.printf( "Switch has been turned : %s\n", (state)? "on" : "off" ); 
      }) ); 


    wemos.begin(); 
    

    
    
    HTTP.begin();
    
}

void loop() {
  ArduinoOTA.handle();
  wemos.loop(); 

}