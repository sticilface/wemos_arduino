

#include "WemosArduino.h"
#include <ESP8266WiFi.h>
#include <functional>



Wemos::Wemos(AsyncWebServer & HTTP) : _HTTP(HTTP), _ip( IPAddress(239, 255, 255, 250)), _deviceName(String())
{

}


bool Wemos::begin()
{
	using namespace std::placeholders;

	if (!WiFi.isConnected())  {
		_Debugf("WIFI NOT CONNECTED: Wemos NOT Started\n");
		return false;
	}

	if (!_UDP.beginMulticast(WiFi.localIP(), _ip, _port)) {
		_Debugf("Connection UDP FAILED\n");
		return false;
	}

	_HTTP.on("/setup.xml", HTTP_ANY, std::bind (&Wemos::_handleSetup, this, _1));
	_HTTP.on("/eventservice.xml", HTTP_ANY, std::bind (&Wemos::_handleEventService, this, _1));


	_HTTP.on("/upnp/control/basicevent1", HTTP_POST, 
	[this](AsyncWebServerRequest * request) {

	},

	[this] (AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

		_Debugf("FILE UPLOAD CALLED\n");
	},

	[this] (AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {

		_Debugf("COMMAND len = %u, index = %u, total = %u\n", len, index, total);

		// Serial.println("BODY");
		// Serial.write(data, len);
		// Serial.println(); 

		if (len == total) {
			data[len] = 0; // null terminate it... should not matter...
			String cmd = (const char *)data;

		bool handled = false;

		WemosHandler * currentHandle = _firstHandle;

		while (currentHandle && !handled) {
			handled =  currentHandle->handle(cmd);
			currentHandle = currentHandle->next();

		}

			request->send(200, "text/plain", "");


		}


	});

	_connected = true;
	return true;

}

void Wemos::loop()
{
	if (_connected) {

		int packetSize = _UDP.parsePacket();

		if (packetSize && packetSize < 2000) {

			char * data = new char[packetSize + 1];

			if (data) {

				memset(data, 0, packetSize + 1);

				_UDP.read(data, packetSize);

				String request = data;

				if (request.indexOf('M-SEARCH') > 0) {

					if (request.indexOf("urn:Belkin:device:**") > 0 ) {

						if (millis() - _responseTimeout > 10000) {
							_Debugf("ECHO Packet From %s:%u, size  %u bytes\n", _UDP.remoteIP().toString().c_str() , _UDP.remotePort() , packetSize);
							_Debugf("Responding to search request ...\n");
							_answerUDP();
							_UDP.flush();
							_responseTimeout = millis();
						} else {
							_Debugf("Response already sent under 1 sec ago\n");
						}

					}
				}

				delete data;

			} else {
				_UDP.flush();
			}
		}
	} //  is connected
}


void Wemos::_answerUDP()
{

	String IP = WiFi.localIP().toString();

	String response =
	    "HTTP/1.1 200 OK\r\n"
	    "CACHE-CONTROL: max-age=86400\r\n"
	    "DATE: Fri, 15 Apr 2016 04:56:29 GMT\r\n"
	    "EXT:\r\n"
	    "LOCATION: http://" + WiFi.localIP().toString() + ":80/setup.xml\r\n"
	    "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
	    "01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n"
	    "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
	    "ST: urn:Belkin:device:**\r\n"
	    "USN: uuid:" + _getID() + "::urn:Belkin:device:**\r\n"
	    "X-User-Agent: redsonic\r\n\r\n";



	// String response = "HTTP/1.1 200 OK\r\n";
	// response += "CACHE-CONTROL: max-age=86400\r\n";
	// response += "DATE: Fri, 15 Apr 2016 04:56:29 GMT\r\n";
	// response += "EXT:\r\n";
	// response += "LOCATION: http://";
	// response += "192.168.1.214";
	// response += ":80/setup.xml\r\n";
	// response += "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n";
	// response += "01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n";
	// response += "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n";
	// response += "ST: urn:Belkin:device:**\r\n";
	// response += "USN: uuid:bums::urn:Belkin:device:**\r\n";
	// response += "X-User-Agent: redsonic\r\n\r\n";


	//     String response = "HTTP/1.1 200 OK\r\nCACHE-CONTROL: max-age=86400\r\nDATE: Fri, 15 Apr 2016 04:56:29 GMT\r\nEXT:\r\nLOCATION: http://192.168.1.214:80/setup.xml\r\nOPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\nSERVER: Unspecified, UPnP/1.0, Unspecified\r\nST: urn:Belkin:device:**\r\nUSN: uuid:bums::urn:Belkin:device:**\r\nX-User-Agent: redsonic\r\n\r\n";

	if (_UDP.beginPacket(_UDP.remoteIP(), _UDP.remotePort())) {
		int count = _UDP.write(response.c_str(), response.length() );
		_UDP.endPacket();
		_Debugf("Response sent to: %s:%u\n", _UDP.remoteIP().toString().c_str(), _UDP.remotePort());

	} else {
		_Debugf("ERROR SENDING UDP PACKET\n");
	}

}


void Wemos::_handleEventService(AsyncWebServerRequest *request)
{
	_Debugf("EventService Recieved\n");
	dump(request);
}


void Wemos::_handleSetup(AsyncWebServerRequest *request)
{
	_Debugf("Setup Request Recieved\n");
	dump(request);


	String reply = "<?xml version=\"1.0\"?>"
	               "<root>"
	               "<device>"
	               "<deviceType>urn:MakerMusings:device:controllee:1</deviceType>"
	               "<friendlyName>" + _deviceName + "</friendlyName>"
	               "<manufacturer>Belkin International Inc.</manufacturer>"
	               "<modelName>Emulated Socket</modelName>"
	               "<modelNumber>3.1415</modelNumber>"
	               "<UDN>uuid:" + _getID() + "</UDN>"
	               "</device>"
	               "</root>\r\n";



	// String setup_xml = "<?xml version=\"1.0\"?>"
	//       "<root>"
	//        "<device>"
	//           "<deviceType>urn:Belkin:device:controllee:1</deviceType>"
	//           "<friendlyName>" + String(_deviceName)  + "</friendlyName>"
	//           "<manufacturer>Belkin International Inc.</manufacturer>"
	//           "<modelName>Emulated Socket</modelName>"
	//           "<modelNumber>3.1415</modelNumber>"
	//           "<UDN>uuid:"+ _getID() +"</UDN>"
	//           "<serialNumber>221517K0101769</serialNumber>"
	//           "<binaryState>0</binaryState>"
	//           "<serviceList>"
	//             "<service>"
	//                 "<serviceType>urn:Belkin:service:basicevent:1</serviceType>"
	//                 "<serviceId>urn:Belkin:serviceId:basicevent1</serviceId>"
	//                 "<controlURL>/upnp/control/basicevent1</controlURL>"
	//                 "<eventSubURL>/upnp/event/basicevent1</eventSubURL>"
	//                 "<SCPDURL>/eventservice.xml</SCPDURL>"
	//             "</service>"
	//         "</serviceList>"
	//         "</device>"
	//       "</root>\r\n"
	//       "\r\n";

	request->send(200, "text/xml", reply);

}


String Wemos::_getID()
{

	uint32_t chipId = ESP.getChipId();
	char uuid[64];
	sprintf_P(uuid, PSTR("38323636-4558-4dda-9188-cda0e6%02x%02x%02x"),
	          (uint16_t) ((chipId >> 16) & 0xff),
	          (uint16_t) ((chipId >>  8) & 0xff),
	          (uint16_t)   chipId        & 0xff);


	return String("Socket-1_0-") + uuid;
	// serial = String(uuid);
	// persistent_uuid = "Socket-1_0-" + serial;
	// device_name = "box";

}

WemosHandler * Wemos::addDevice(WemosHandler * device)
{

	_Debugf("Adding Device\n");


	if (!_firstHandle) {
		_Debugf("Adding 1st Device\n");

		_firstHandle = device;
	} else {

		_Debugf("start go through\n");
		uint32_t count = 2;

		WemosHandler * current = _firstHandle;

		while (current->next()) {
			current = current->next();
			count++;
		}

		current->next(device);
		_Debugf("Adding %u device\n", count);

	}

	_Debugf("Done Adding Devices\n");


	return device;

}

void Wemos::dump(AsyncWebServerRequest *request)
{
	//List all collected headers
	_Debugf("HEADERS ---\n");
	int headers = request->headers();
	int i;
	for (i = 0; i < headers; i++) {
		AsyncWebHeader* h = request->getHeader(i);
		_Debugf("HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
	}
	_Debugf("PARAMS ---\n");
//List all parameters
	int params = request->params();
	for (int i = 0; i < params; i++) {
		AsyncWebParameter* p = request->getParam(i);
		if (p->isFile()) { //p->isPost() is also true
			_Debugf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
		} else if (p->isPost()) {
			_Debugf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
		} else {
			_Debugf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
		}
	}

	//_Debugf("BODY ----- ");


}

//  parses the request to decide if it is this device.  then calls its callback with the corect stuff
//  this does not work at all but im laying the ground work to support multiple switches or devices...  hopefully through a bridge
bool WemosSwitch::handle(String cmd)
{

	_Debugf("WemosSwitch::handle\n");
	_Debugf("cmd = %s\n", cmd.c_str()); 

			if (cmd.indexOf("<BinaryState>1</BinaryState>") > 0 ) {
				_Debugf("Got Turn on request\n");
				if (_cb) {
					_cb(true);
						return true;
				}
			} else if (cmd.indexOf("<BinaryState>0</BinaryState>") > 0 ) {
				_Debugf("Got Turn off request\n");
				if (_cb) {
					_cb(false);
						return true;

				}
			}
return false; 

}