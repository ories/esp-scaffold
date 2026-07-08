#include "WebUI.h"

#include <Update.h>

#include "Logger.h"
#include "Theme.h"
#include "WiFiSetup.h"

#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif
#ifndef BUILD_TIMESTAMP
#define BUILD_TIMESTAMP "unknown"
#endif

namespace {

const char PAGE_SHELL[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>%TITLE%</title>
<style>
%THEME%
nav {
  display: flex;
  gap: 4px;
  padding: 12px 16px;
  background: var(--navy);
}
nav a {
  color: var(--pale);
  text-decoration: none;
  padding: 8px 14px;
  border-radius: 6px;
  font-size: 14px;
}
nav a.active { background: var(--water); color: var(--white); }
nav a:hover { background: rgba(255, 255, 255, 0.12); }
pre#log {
  background: var(--inset);
  border: 1px solid var(--pale);
  border-radius: 8px;
  padding: 14px;
  height: 60vh;
  overflow-y: auto;
  font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 12.5px;
  line-height: 1.5;
  white-space: pre-wrap;
  word-break: break-word;
}
footer {
  padding: 10px 16px;
  color: var(--muted);
  font-size: 12px;
  text-align: center;
}
footer button {
  background: none;
  border: none;
  color: var(--water);
  font-size: 12px;
  text-decoration: underline;
  cursor: pointer;
  padding: 0;
  margin-left: 6px;
}
footer button:hover { color: var(--navy); }
</style>
</head>
<body>
<nav>
  <a href="/" class="%NAV_LOGS%">Logs</a>
  <a href="/update" class="%NAV_OTA%">Firmware Update</a>
</nav>
<div class="page wide">
%BODY%
</div>
<footer>
  build %COMMIT% &middot; %BUILD_TIME%
  &middot;
  <form method="POST" action="/forget-wifi" style="display:inline"
        onsubmit="return confirm('Forget WiFi credentials and reboot into setup mode?')">
    <button type="submit">Forget WiFi</button>
  </form>
</footer>
</body>
</html>
)HTML";

const char LOGS_BODY[] PROGMEM = R"HTML(<div class="box">
<div class="box-title">Live Logs</div>
<pre id="log">Connecting&hellip;</pre>
</div>
<script>
  const el = document.getElementById('log');
  const es = new EventSource('/events');
  function append(text) {
    const atBottom = el.scrollTop + el.clientHeight >= el.scrollHeight - 4;
    el.textContent += text + "\n";
    if (atBottom) el.scrollTop = el.scrollHeight;
  }
  es.addEventListener('history', (e) => {
    el.textContent = e.data + "\n";
    el.scrollTop = el.scrollHeight;
  });
  es.addEventListener('log', (e) => append(e.data));
</script>
)HTML";

const char OTA_BODY[] PROGMEM = R"HTML(<div class="box">
<div class="box-title">Firmware Update</div>
<form id="otaForm">
  <input type="file" id="firmware" name="firmware" accept=".bin">
  <button type="submit">Upload</button>
</form>
<progress id="progressBar" value="0" max="100" style="display:none;width:100%;margin-top:12px"></progress>
<div id="status" style="margin-top:8px;font-size:14px"></div>
</div>
<script>
  const form = document.getElementById('otaForm');
  const bar = document.getElementById('progressBar');
  const status = document.getElementById('status');
  form.addEventListener('submit', (e) => {
    e.preventDefault();
    const file = document.getElementById('firmware').files[0];
    if (!file) return;
    const data = new FormData();
    data.append('firmware', file);
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/update');
    xhr.upload.addEventListener('progress', (evt) => {
      if (!evt.lengthComputable) return;
      bar.style.display = 'block';
      bar.value = (evt.loaded / evt.total) * 100;
    });
    xhr.onload = () => {
      status.textContent = xhr.status === 200
        ? xhr.responseText + ' Rebooting...'
        : 'Update failed: ' + xhr.responseText;
    };
    xhr.onerror = () => { status.textContent = 'Upload failed'; };
    xhr.send(data);
  });
</script>
)HTML";

String renderLogsPage() {
  String page = String((const __FlashStringHelper *)PAGE_SHELL);
  page.replace("%THEME%", String((const __FlashStringHelper *)THEME_CSS));
  page.replace("%TITLE%", "Logs");
  page.replace("%BODY%", String((const __FlashStringHelper *)LOGS_BODY));
  page.replace("%NAV_LOGS%", "active");
  page.replace("%NAV_OTA%", "");
  page.replace("%COMMIT%", GIT_COMMIT);
  page.replace("%BUILD_TIME%", BUILD_TIMESTAMP);
  return page;
}

String renderOTAPage() {
  String page = String((const __FlashStringHelper *)PAGE_SHELL);
  page.replace("%THEME%", String((const __FlashStringHelper *)THEME_CSS));
  page.replace("%TITLE%", "Firmware Update");
  page.replace("%BODY%", String((const __FlashStringHelper *)OTA_BODY));
  page.replace("%NAV_LOGS%", "");
  page.replace("%NAV_OTA%", "active");
  page.replace("%COMMIT%", GIT_COMMIT);
  page.replace("%BUILD_TIME%", BUILD_TIMESTAMP);
  return page;
}

volatile bool otaRebootPending = false;
unsigned long otaRebootAt = 0;

void handleOTAUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,
                      size_t len, bool final) {
  if (index == 0) {
    logger.log("OTA update started: " + filename);
    size_t size = request->contentLength() ? request->contentLength() : UPDATE_SIZE_UNKNOWN;
    if (!Update.begin(size, U_FLASH)) {
      Update.printError(Serial);
      logger.log("OTA update failed to start: " + String(Update.errorString()));
    }
  }

  if (Update.isRunning() && Update.write(data, len) != len) {
    Update.printError(Serial);
  }

  if (final) {
    if (Update.isRunning() && Update.end(true)) {
      logger.log("OTA update finished successfully, size=" + String(index + len));
    } else {
      Update.printError(Serial);
      logger.log("OTA update failed: " + String(Update.errorString()));
    }
  }
}

void handleOTAComplete(AsyncWebServerRequest *request) {
  bool success = !Update.hasError();
  request->send(success ? 200 : 500, "text/plain",
                success ? "Update successful." : Update.errorString());
  if (success) {
    otaRebootAt = millis() + 500;  // give the response time to flush before rebooting
    otaRebootPending = true;
  }
}

}  // namespace

void webUILoop() {
  if (otaRebootPending && millis() >= otaRebootAt) {
    ESP.restart();
  }
}

void setupWebUI(AsyncWebServer &server) {
  logger.begin(server);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", renderLogsPage());
  });

  server.on("/forget-wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    logger.log("WiFi credentials cleared, rebooting into setup portal");
    request->send(200, "text/plain", "Forgotten. Rebooting into WiFi setup mode...");
    WiFiSetup::forget();
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", renderOTAPage());
  });

  server.on(
      "/update", HTTP_POST, handleOTAComplete, [](AsyncWebServerRequest *request, const String &filename,
                                                    size_t index, uint8_t *data, size_t len, bool final) {
        handleOTAUpload(request, filename, index, data, len, final);
      });
}
