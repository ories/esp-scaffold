#pragma once

#include <ESPAsyncWebServer.h>

// Wires up the HTTP routes: "/" live log viewer, "/update" OTA page
// (ElegantOTA), plus the shared nav/footer chrome.
void setupWebUI(AsyncWebServer &server);
