#include <Arduino.h>

/*
 * ESP32-C5 + XPT2046 standalone raw-touch diagnostic
 *
 * Current wiring:
 *   XPT2046 T_CLK -> GPIO5
 *   XPT2046 T_DIN -> GPIO7   (ESP MOSI -> touch controller)
 *   XPT2046 T_DO  -> GPIO25  (touch controller -> ESP MISO)
 *   XPT2046 T_CS  -> GPIO1
 *   XPT2046 T_IRQ -> GPIO4
 *
 * LCD is intentionally NOT initialized.
 * This firmware tests the touch controller only.
 */

static constexpr int T_CLK = 5;
static constexpr int T_DIN = 7;
static constexpr int T_DO  = 25;
static constexpr int T_CS  = 1;
static constexpr int T_IRQ = 4;

static inline void clkHigh() {
  digitalWrite(T_CLK, HIGH);
  delayMicroseconds(3);
}

static inline void clkLow() {
  digitalWrite(T_CLK, LOW);
  delayMicroseconds(3);
}

static uint16_t xptRead12(uint8_t command) {
  digitalWrite(T_CS, LOW);

  // Send the 8-bit XPT2046 command, MSB first.
  for (int bit = 7; bit >= 0; --bit) {
    digitalWrite(T_DIN, (command >> bit) & 0x01);
    clkHigh();
    clkLow();
  }

  // Read the 16 clocks returned by XPT2046.
  // The useful conversion result is the 12-bit value in bits 14..3.
  uint16_t raw = 0;
  for (int i = 0; i < 16; ++i) {
    clkHigh();
    raw = (raw << 1) | (digitalRead(T_DO) ? 1 : 0);
    clkLow();
  }

  digitalWrite(T_CS, HIGH);
  return (raw >> 3) & 0x0FFF;
}

static void printPinState() {
  Serial.printf(
    "[TOUCH] IRQ=%d  RAW_X=%4u  RAW_Y=%4u  DO=%d\n",
    digitalRead(T_IRQ),
    xptRead12(0xD0),   // X position command
    xptRead12(0x90),   // Y position command
    digitalRead(T_DO)
  );
}

void setup() {
  Serial.begin(115200);

  // Give native USB CDC a moment to enumerate.
  delay(1500);

  pinMode(T_CLK, OUTPUT);
  pinMode(T_DIN, OUTPUT);
  pinMode(T_DO, INPUT);
  pinMode(T_CS, OUTPUT);
  pinMode(T_IRQ, INPUT_PULLUP);

  digitalWrite(T_CS, HIGH);
  digitalWrite(T_CLK, LOW);
  digitalWrite(T_DIN, LOW);

  Serial.println();
  Serial.println("==============================================");
  Serial.println(" ESP32-C5 / XPT2046 STANDALONE TOUCH TEST");
  Serial.println("==============================================");
  Serial.println("T_CLK : GPIO5");
  Serial.println("T_DIN : GPIO7");
  Serial.println("T_DO  : GPIO25");
  Serial.println("T_CS  : GPIO1");
  Serial.println("T_IRQ : GPIO4");
  Serial.println();
  Serial.println("LCD is not used by this test.");
  Serial.println("Touch the panel and move your finger.");
  Serial.println("Expected:");
  Serial.println("  - IRQ normally 1, pressed normally 0");
  Serial.println("  - RAW_X / RAW_Y should change with finger position");
  Serial.println("  - readings are forced even if IRQ is wrong/unconnected");
  Serial.println();
}

void loop() {
  printPinState();
  delay(200);
}
