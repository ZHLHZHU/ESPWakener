#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFiUDP.h>
#include <config.h>

extern WiFiUDP udp;

void wakeOnLan(const IPAddress& dstIp, byte *macAddress);

String wakeByMacStr(const String& macStr);