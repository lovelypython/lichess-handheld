#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "config.h"

class ST7796Display {
 public:
  explicit ST7796Display(SPIClass& spi) : spi_(spi) {}
  void begin();
  void sleep();
  void wake();
  void fillScreen(uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawLine(int x0, int y0, int x1, int y1, uint16_t color);
  void drawCircle(int x0, int y0, int r, uint16_t color);
  void fillCircle(int x0, int y0, int r, uint16_t color);
  void drawText(int x, int y, const String& s, uint16_t fg, uint16_t bg, int scale=1);
  void drawChar(int x, int y, char c, uint16_t fg, uint16_t bg, int scale=1);
  void drawRGB565Masked(int x, int y, const uint16_t* pixels, const uint8_t* mask,
                        int w, int h);

  static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }

 private:
  SPIClass& spi_;
  void select();
  void deselect();
  void cmd(uint8_t c);
  void data(uint8_t d);
  void data16(uint16_t d);
  void setWindow(int16_t x, int16_t y, int16_t w, int16_t h);
  void resetPanel();
};
