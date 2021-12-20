#include <web.h>

AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found.");
}

void resConfig(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  StaticJsonDocument<512> doc = convertState();
  serializeJson(doc, *response);
  request->send(response);
  doc.clear();
}

AsyncCallbackJsonWebHandler *updateConfigHandlerFactory()
{
  return new AsyncCallbackJsonWebHandler("/update-config", [](AsyncWebServerRequest *request, JsonVariant &json)
                                         {
                                           StaticJsonDocument<512> doc;
                                           if (json.is<JsonObject>())
                                           {
                                             doc = json.as<JsonObject>();
                                           }
                                           refreshState(doc);
                                           saveConfig();
                                           request->send(200, "text/plain", "ok");
                                         });
}

void resFirmwareVersion(AsyncWebServerRequest *request){
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  StaticJsonDocument<512> doc = convertState();
  doc["code"] = 200;
  doc["message"] = "success";
  doc["firmwareVersion"] = FIRMWARE_VERSION;
  serializeJson(doc, *response);
  request->send(response);
  doc.clear();
}

void initWebServer()
{
  server.serveStatic("/", LittleFS, "/www").setDefaultFile("index.html");
  server.on("/config", resConfig);
  server.on("/version",resFirmwareVersion);
  server.addHandler(updateConfigHandlerFactory());
  server.onNotFound(notFound);
  server.begin();
}