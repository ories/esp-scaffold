#pragma once

// Connects using WiFi credentials stored in NVS. If none are stored, or the
// stored ones fail to connect, blocks running a captive config portal
// (AP + DNS + web form) until new credentials are submitted, then reboots.
// Only returns once STA is connected.
class WiFiSetup {
 public:
  void connectOrPortal();

  // Wipes stored credentials and reboots into the portal.
  static void forget();

 private:
  bool tryStoredCredentials();
  void startPortal();
};

extern WiFiSetup wifiSetup;
