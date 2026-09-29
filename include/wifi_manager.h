#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "config.h"

class WiFiManagerLite {
 public:
  static constexpr int MAX_SCAN_RESULTS = 16;

  void begin();
  bool autoConnect(uint32_t perNetworkTimeoutMs = 7000);
  int scan();
  int scanCount() const { return scanCount_; }
  String scanSSID(int index) const;
  int scanRSSI(int index) const;
  bool scanSecure(int index) const;
  bool connectAndSave(const String& ssid, const String& password,
                      uint32_t timeoutMs = 15000);
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
  String scanSSIDs_[MAX_SCAN_RESULTS];
  int scanRSSIs_[MAX_SCAN_RESULTS] = {};
  bool scanSecure_[MAX_SCAN_RESULTS] = {};
  int scanCount_ = 0;
  void loadExtra();
  bool connectOne(const char* ssid, const char* pass, uint32_t timeoutMs);
};
