#pragma once

#include <ESPAsyncWebServer.h>

// Wires up the HTTP routes: "/" live log viewer, "/update" OTA page
// (Update.h based), plus the shared nav/footer chrome.
void setupWebUI(AsyncWebServer &server);

// Must be called from the main loop(); reboots the device once a
// completed OTA upload has had time to flush its HTTP response.
void webUILoop();
