#include <wol.h>

WiFiUDP udp;

void wakeOnLan(const IPAddress& dstIp, byte *macAddress)
{
  byte sync[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  udp.beginPacket(dstIp, config.wolPort);
  udp.write(sync, sizeof(sync));
  for (char i = 0; i < 16; i++)
    udp.write(macAddress, sizeof(byte) * MAC_ADDRESS_BYTE);
  udp.endPacket();
}

String wakeByMacStr(const String& macStr)
{
  if (macStr.isEmpty())
  {
    return "mac is empty";
  }
  int colonIndex = macStr.indexOf(':');
  int minusIndex = macStr.indexOf('-');
  if (!(colonIndex ^ minusIndex))
  {
    return "mac address format error.";
  }
  const char *separator = (colonIndex > 0 ? ":" : "-");

  char *str = new char[macStr.length() + 1];
  macStr.toCharArray(str, macStr.length() + 1, 0);
  byte mac[6];
  char *ptr;
  mac[0] = strtol(strtok(str, separator), &ptr, HEX);
  for (uint8_t i = 1; i < 6; i++)
    mac[i] = strtol(strtok(nullptr, separator), &ptr, HEX);

  wakeOnLan(broadcastIP, mac);
  free(str);
  return "";
}
