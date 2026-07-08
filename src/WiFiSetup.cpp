#include "WiFiSetup.h"

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <WiFi.h>

#include "Theme.h"

WiFiSetup wifiSetup;

namespace {

constexpr const char *kNamespace = "wifi";
constexpr uint32_t kConnectTimeoutMs = 15000;
constexpr uint32_t kSaveRebootDelayMs = 1500;

DNSServer dnsServer;
AsyncWebServer portalServer(80);
bool saveRequested = false;
uint32_t saveRequestedAt = 0;

String apName() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[24];
  snprintf(buf, sizeof(buf), "ESP32-Setup-%02X%02X", mac[4], mac[5]);
  return String(buf);
}

String htmlEscape(const String &s) {
  String out;
  out.reserve(s.length());
  for (char c : s) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      default: out += c;
    }
  }
  return out;
}

String renderPortalPage() {
  String options;
  int n = WiFi.scanComplete();
  if (n < 0) n = 0;
  for (int i = 0; i < n; i++) {
    options += "<option value=\"" + htmlEscape(WiFi.SSID(i)) + "\">";
  }

  String page = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WiFi Setup</title>
<style>
%THEME%
body {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
}
.page { padding: 20px; }
h1 { color: var(--navy); font-size: 18px; margin: 0 0 16px; }
label { display: block; font-size: 13px; color: var(--muted); margin: 14px 0 6px; }
input {
  width: 100%;
  padding: 10px;
  border-radius: 6px;
  border: 1px solid var(--pale);
  background: var(--inset);
  color: var(--text);
  font-size: 14px;
}
button {
  width: 100%;
  margin-top: 20px;
  padding: 11px;
  border: none;
  border-radius: 6px;
  background: var(--water);
  color: var(--white);
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
}
button:hover { background: var(--navy); }
</style>
</head>
<body>
<div class="page">
<form class="box" method="POST" action="/save">
  <h1>Connect device to WiFi</h1>
  <label for="ssid">Network name</label>
  <input list="networks" id="ssid" name="ssid" placeholder="SSID" autocomplete="off" required>
  <datalist id="networks">%OPTIONS%</datalist>
  <label for="password">Password</label>
  <input type="password" id="password" name="password" placeholder="Password" autocomplete="off">
  <button type="submit">Save &amp; Connect</button>
</form>
</div>
</body>
</html>
)HTML";
  page.replace("%THEME%", String((const __FlashStringHelper *)THEME_CSS));
  page.replace("%OPTIONS%", options);
  return page;
}

}  // namespace

bool WiFiSetup::tryStoredCredentials() {
  Preferences prefs;
  prefs.begin(kNamespace, /*readOnly=*/true);
  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("password", "");
  prefs.end();

  if (ssid.isEmpty()) {
    Serial.println("No stored WiFi credentials");
    return false;
  }

  Serial.printf("Connecting to stored network \"%s\"...\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < kConnectTimeoutMs) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect with stored credentials");
    return false;
  }

  Serial.print("Connected, IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

void WiFiSetup::startPortal() {
  WiFi.mode(WIFI_AP_STA);
  String name = apName();
  WiFi.softAP(name.c_str());
  delay(100);

  Serial.println("Scanning nearby WiFi networks...");
  int found = WiFi.scanNetworks();
  Serial.printf("Found %d networks\n", found > 0 ? found : 0);

  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(53, "*", apIP);

  portalServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", renderPortalPage());
  });

  portalServer.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid", true)) {
      request->send(400, "text/plain", "Missing ssid");
      return;
    }
    String ssid = request->getParam("ssid", true)->value();
    String password = request->hasParam("password", true) ? request->getParam("password", true)->value() : "";

    Preferences prefs;
    prefs.begin(kNamespace, /*readOnly=*/false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();

    request->send(200, "text/html",
                  "<html><body style=\"font-family:sans-serif;background:#EEF5FB;color:#1A2B3C;"
                  "display:flex;align-items:center;justify-content:center;height:100vh;margin:0\">"
                  "<p>Saved. Rebooting&hellip;</p></body></html>");
    saveRequested = true;
    saveRequestedAt = millis();
  });

  portalServer.onNotFound([](AsyncWebServerRequest *request) { request->redirect("http://" + WiFi.softAPIP().toString() + "/"); });

  portalServer.begin();

  Serial.println("Config portal ready:");
  Serial.println("  connect to WiFi \"" + name + "\"");
  Serial.println("  then open http://" + apIP.toString());

  while (true) {
    dnsServer.processNextRequest();
    if (saveRequested && millis() - saveRequestedAt > kSaveRebootDelayMs) {
      ESP.restart();
    }
    delay(5);
  }
}

void WiFiSetup::connectOrPortal() {
  if (tryStoredCredentials()) return;
  startPortal();  // never returns; device reboots once credentials are saved
}

void WiFiSetup::forget() {
  Preferences prefs;
  prefs.begin(kNamespace, /*readOnly=*/false);
  prefs.clear();
  prefs.end();
  delay(500);  // let the HTTP response flush before rebooting
  ESP.restart();
}
