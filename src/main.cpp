#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "Logger.h"
#include "WebUI.h"
#include "WiFiSetup.h"

namespace {

AsyncWebServer server(80);

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);

  wifiSetup.connectOrPortal();  // blocks until connected; may reboot the device

  logger.log("WiFi connected, IP: " + WiFi.localIP().toString());

  setupWebUI(server);
  server.begin();

  logger.log("HTTP server started on port 80");
  logger.log(String("Build ") + GIT_COMMIT + " @ " + BUILD_TIMESTAMP);
}

void loop() {
  webUILoop();  // reboots after a completed OTA upload

  static unsigned long lastHeartbeat = 0;
  unsigned long now = millis();
  if (now - lastHeartbeat >= 10000) {
    lastHeartbeat = now;
    logger.log("heartbeat, uptime=" + String(now / 1000) + "s, free heap=" + String(ESP.getFreeHeap()));
  }
}
