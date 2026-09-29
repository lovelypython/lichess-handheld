#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
#include "config.h"
#include "display.h"

struct TouchPoint {
  bool pressed = false;
  int16_t x = 0;
  int16_t y = 0;
  uint16_t rawX = 0;
  uint16_t rawY = 0;
};

class XPT2046Touch {
 public:
  explicit XPT2046Touch(SPIClass& spi) : spi_(spi) {}
  void begin();
  bool readRaw(uint16_t& x, uint16_t& y);
  TouchPoint read();
  bool calibrated() const { return calibrated_; }
  bool usingDefaultCalibration() const { return usingDefaultCalibration_; }
  void runCalibration(ST7796Display& tft);
  void clearCalibration();
  void useDefaultCalibration(bool persist = true);

 private:
  SPIClass& spi_;
  Preferences prefs_;
  bool calibrated_ = false;
  bool usingDefaultCalibration_ = false;
  float a_=0, b_=0, c_=0, d_=0, e_=0, f_=0;

  uint16_t read12(uint8_t command);
  void load();
  void save();
  bool solveAffine(const float raw[][2], const float scr[][2], size_t count);
};
