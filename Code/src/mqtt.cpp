#include <mqtt.h>

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  if (config.mqttTopic != topic)
  {
    Serial.println("receive out of topic message");
    return;
  }
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error)
  {
    Serial.println("Failed to parse JSON");
    return;
  }
  String action = doc["action"] | "";
  String sender = doc["sender"] | "";
  if (sender == DEVICE_NAME)
  {
    return;
  }
  StaticJsonDocument<256> res;
  res["sender"] = DEVICE_NAME;
  char buffer[256];
  if (action == "wake")
  {
    String mac = doc["mac"] | "";
    if (mac.isEmpty())
    {
      res["code"] = 400;
      res["message"] = "mac address is empty.";
      serializeJson(res, buffer);
      mqtt.publish(config.mqttTopic.c_str(), buffer, true);
      return;
    }
    if(!isMacAddress(mac)){
        res["code"] = 400;
        res["message"] = "mac address invalid.";
        serializeJson(res, buffer);
        mqtt.publish(config.mqttTopic.c_str(), buffer, true);
        return;
    }
    String wakeRes = wakeByMacStr(mac);
    // wake failed
    if(!wakeRes.isEmpty())
    {
      res["code"] = 400;
      res["message"] = wakeRes;
      serializeJson(res, buffer);
      mqtt.publish(config.mqttTopic.c_str(), buffer, true);
      return;
    }
    //success to wake
    res["code"] = 200;
    res["message"] = "success,already send magic package to " + mac;
    serializeJson(res, buffer);
    mqtt.publish(config.mqttTopic.c_str(), buffer, true);
    return;
  }
  if (action == "info")
  {
    resDeviceInfo("resInfo");
    return;
  }
}

void mqttReconnect()
{
  // Loop until we're reconnected
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttConnect())
    {
      Serial.println("connected");
      //resubscribe
      mqtt.subscribe(config.mqttTopic.c_str(), 0);
      resDeviceInfo("join");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 10 seconds before retrying
        delay(10000);
    }

  }
}

bool mqttConnect()
{
  String username = config.mqttUsername;
  String password = config.mqttPass;
  return mqtt.connect((String(DEVICE_NAME)+"_"+ random(2021)).c_str(),
                      username.isEmpty() ? nullptr : username.c_str(),
                      password.isEmpty() ? nullptr : password.c_str());
}

void initMqtt()
{
  mqtt.setServer(config.mqttHost.c_str(), config.mqttPort);
  mqtt.setCallback(mqttCallback);
}

void resDeviceInfo(const char *action){
    StaticJsonDocument<256> res;
    res["action"] = action;
    res["sender"] = DEVICE_NAME;
    res["deviceName"] = DEVICE_NAME;
    res["ip"] = WiFi.localIP().toString();
    res["ssid"] = WiFi.SSID();
    res["rssi"] = WiFi.RSSI();
    res["version"] = FIRMWARE_VERSION;
    char buffer[256];
    serializeJson(res, buffer);
    mqtt.publish(config.mqttTopic.c_str(), buffer, true);
}