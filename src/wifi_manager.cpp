#include "wifi_manager.h"

void WiFiManagerLite::loadExtra() {
  // Open read/write so a fresh board creates the namespace instead of logging
  // an expected NVS NOT_FOUND error on first boot.
  prefs_.begin("wifi-extra", false);
  extraSSID_ = prefs_.getString("ssid", "");
  extraPassword_ = prefs_.getString("pass", "");
  prefs_.end();
}

int WiFiManagerLite::scan() {
  scanCount_ = 0;
  int n = WiFi.scanNetworks(false, true);
  if (n < 0) return 0;

  // Keep the strongest instance of each SSID and cap the list so the UI stays
  // small and deterministic.
  for (int i = 0; i < n && scanCount_ < MAX_SCAN_RESULTS; ++i) {
    String ssid = WiFi.SSID(i);
    if (!ssid.length()) continue;
    bool duplicate = false;
    for (int j = 0; j < scanCount_; ++j) {
      if (scanSSIDs_[j] == ssid) { duplicate = true; break; }
    }
    if (duplicate) continue;
    scanSSIDs_[scanCount_] = ssid;
    scanRSSIs_[scanCount_] = WiFi.RSSI(i);
    scanSecure_[scanCount_] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    ++scanCount_;
  }
  WiFi.scanDelete();
  return scanCount_;
}

String WiFiManagerLite::scanSSID(int index) const {
  return (index >= 0 && index < scanCount_) ? scanSSIDs_[index] : String();
}

int WiFiManagerLite::scanRSSI(int index) const {
  return (index >= 0 && index < scanCount_) ? scanRSSIs_[index] : -127;
}

bool WiFiManagerLite::scanSecure(int index) const {
  return index >= 0 && index < scanCount_ && scanSecure_[index];
}

bool WiFiManagerLite::connectAndSave(const String& ssid, const String& password,
                                     uint32_t timeoutMs) {
  if (!ssid.length()) return false;
  if (!connectOne(ssid.c_str(), password.c_str(), timeoutMs)) return false;
  return addOrReplaceExtra(ssid, password);
}

void WiFiManagerLite::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  loadExtra();
}

bool WiFiManagerLite::connectOne(const char* ssid, const char* pass, uint32_t timeoutMs) {
  Serial.printf("[WiFi] trying %s\n", ssid);
  WiFi.disconnect(false, false);
  delay(100);
  WiFi.begin(ssid, pass);
  uint32_t start=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-start<timeoutMs) delay(80);
  if(WiFi.status()==WL_CONNECTED){
    Serial.printf("[WiFi] connected %s, IP %s, RSSI %d dBm\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
    return true;
  }
  return false;
}

bool WiFiManagerLite::autoConnect(uint32_t perNetworkTimeoutMs) {
  if(WiFi.status()==WL_CONNECTED) return true;

  int n=WiFi.scanNetworks(false,true);
  Serial.printf("[WiFi] scan found %d networks\n", n);

  auto visible=[&](const String& s){
    for(int i=0;i<n;i++) if(WiFi.SSID(i)==s) return true;
    return false;
  };

  for(size_t i=0;i<BUILTIN_WIFI_COUNT;i++){
    if(visible(BUILTIN_WIFI[i].ssid) &&
       connectOne(BUILTIN_WIFI[i].ssid, BUILTIN_WIFI[i].password, perNetworkTimeoutMs)) {
      WiFi.scanDelete();
      return true;
    }
  }
  if(extraSSID_.length() && visible(extraSSID_) &&
     connectOne(extraSSID_.c_str(), extraPassword_.c_str(), perNetworkTimeoutMs)) {
    WiFi.scanDelete();
    return true;
  }

  WiFi.scanDelete();
  return false;
}

bool WiFiManagerLite::addOrReplaceExtra(const String& ssid, const String& password) {
  if(!ssid.length()) return false;
  prefs_.begin("wifi-extra", false);
  prefs_.putString("ssid",ssid);
  prefs_.putString("pass",password);
  prefs_.end();
  extraSSID_=ssid; extraPassword_=password;
  return true;
}
void WiFiManagerLite::clearExtra() {
  prefs_.begin("wifi-extra",false);prefs_.clear();prefs_.end();
  extraSSID_="";extraPassword_="";
}
String WiFiManagerLite::currentSSID() const { return WiFi.status()==WL_CONNECTED?WiFi.SSID():"offline"; }
String WiFiManagerLite::ip() const { return WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():"-"; }
int WiFiManagerLite::rssi() const { return WiFi.status()==WL_CONNECTED?WiFi.RSSI():0; }

void WiFiManagerLite::scanToSerial() {
  int n=WiFi.scanNetworks(false,true);
  Serial.printf("Found %d networks:\n",n);
  for(int i=0;i<n;i++) Serial.printf("  %2d  %4d dBm  %s\n",i,WiFi.RSSI(i),WiFi.SSID(i).c_str());
  WiFi.scanDelete();
}
