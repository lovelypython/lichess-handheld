#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "config.h"

class WiFiManagerLite {
 public:
  void begin();
  bool autoConnect(uint32_t perNetworkTimeoutMs = 7000);
  bool addOrReplaceExtra(const String& ssid, const String& password);
  void clearExtra();
  String currentSSID() const;
  String ip() const;
  int rssi() const;
  void scanToSerial();

 private:
  Preferences prefs_;
  String extraSSID_;
  String extraPassword_;
  void loadExtra();
  bool connectOne(const char* ssid, const char* pass, uint32_t timeoutMs);
};
