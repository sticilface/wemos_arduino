/*
*
*	A lib to control a sketch using wemos.  Handles auto discovery.
*   Requires espAsync and UDP libs.  and for sketch to be connected to wifi....
*
*
*/

#pragma once

#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <functional>


// enum WemosDevice {
// 	WEMOS_SWITCH = 0,
// 	WEMOS_RGBLED;
// }

//typedef std::function<void(bool)> WemosCallbackSwitch;

//#define DebugWemos Serial 

#ifdef DebugWemos
	#define _Debugf(...) DebugWemos.printf(__VA_ARGS__)
#else
	#define _Debugf(...) {}
#endif

class WemosHandler
{
public:
	//typedef std::function<void(WemosEvent)> WemosCallback;

	virtual bool handle(String cmd) { return false; } 
	virtual void addDescriptors(AsyncWebServerRequest *request) { return; } 


	WemosHandler * next() { return _next; }
	void next(WemosHandler * next) { _next = next;}

private:
	WemosHandler * _next{nullptr};

};


class WemosSwitch : public WemosHandler
{
public:
	typedef std::function<void(bool)> WemosCallbackSwitch;
	WemosSwitch(const char * name, WemosCallbackSwitch cb): _name(name), _cb(cb) {}  
	bool handle(String cmd) override; 


private:
	const char * _name{nullptr}; 
	WemosCallbackSwitch _cb; 
};





class WemosRGB : public WemosHandler
{
public:
	struct WemosRGBdata {
		bool state{false};
		uint8_t color[3] = {0};
	};
	typedef std::function<void(WemosRGBdata)> WemosCallbackRGB;

	WemosRGB(const char * name, WemosCallbackRGB cb): _name(name), _cb(cb) {}  
	bool handle(String cmd) override {}; 


private:
	const char * _name{nullptr}; 
	WemosCallbackRGB _cb; 
};


class Wemos
{

public:

	Wemos(AsyncWebServer & HTTP);
	~Wemos() {}
	WemosHandler * addDevice(WemosHandler * device);
	bool begin();
	void loop();
	static void dump(AsyncWebServerRequest *request);
	void name(String name) { _deviceName = name; }
	String name() { return _deviceName ;}

private:
	void _answerUDP();
	void _handleEventService(AsyncWebServerRequest *request); 
	void _handleSetup(AsyncWebServerRequest *request); 

	String _getID(); 


	WiFiUDP _UDP;
	bool _enabled{false};
	bool _connected{false};
	IPAddress _ip;
	uint16_t _port{1900};
	AsyncWebServer & _HTTP;

	WemosHandler * _firstHandle{nullptr};

	String _deviceName; 
	uint32_t _responseTimeout{0}; 
};