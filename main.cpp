#include <Arduino.h>

// XPT2046 independent software-SPI wiring
#define T_CLK  5
#define T_DIN  7
#define T_DO   25
#define T_CS   1
#define T_IRQ  4

static uint16_t readTouch12(uint8_t command) {
  digitalWrite(T_CS, LOW);

  // Send command, MSB first
  for (int i = 7; i >= 0; --i) {
    digitalWrite(T_DIN, (command >> i) & 1);

    digitalWrite(T_CLK, HIGH);
    delayMicroseconds(3);
    digitalWrite(T_CLK, LOW);
    delayMicroseconds(3);
  }

  uint16_t value = 0;

  for (int i = 0; i < 16; ++i) {
    digitalWrite(T_CLK, HIGH);
    delayMicroseconds(3);

    value <<= 1;
    if (digitalRead(T_DO)) value |= 1;

    digitalWrite(T_CLK, LOW);
    delayMicroseconds(3);
  }

  digitalWrite(T_CS, HIGH);

  return (value >> 3) & 0x0FFF;
}

void setup() {
  Serial.begin(115200);
  delay(1200);

  pinMode(T_CLK, OUTPUT);
  pinMode(T_DIN, OUTPUT);
  pinMode(T_DO, INPUT);
  pinMode(T_CS, OUTPUT);
  pinMode(T_IRQ, INPUT_PULLUP);

  digitalWrite(T_CS, HIGH);
  digitalWrite(T_CLK, LOW);
  digitalWrite(T_DIN, LOW);

  Serial.println();
  Serial.println("=================================");
  Serial.println(" XPT2046 RAW TOUCH TEST / ESP32-C5");
  Serial.println("=================================");
  Serial.println("T_CLK = GPIO5");
  Serial.println("T_DIN = GPIO7");
  Serial.println("T_DO  = GPIO25");
  Serial.println("T_CS  = GPIO1");
  Serial.println("T_IRQ = GPIO4");
  Serial.println();
  Serial.println("Press the touchscreen and move your finger.");
  Serial.println("Expected: IRQ 1 -> 0 and RAW X/Y should change.");
  Serial.println();
}

void loop() {
  const int irq = digitalRead(T_IRQ);

  // Read regardless of IRQ so a bad IRQ wire cannot hide SPI activity.
  const uint16_t x = readTouch12(0xD0);
  delayMicroseconds(100);
  const uint16_t y = readTouch12(0x90);

  Serial.printf(
    "[TOUCH] IRQ=%d  RAW_X=%4u  RAW_Y=%4u  DO=%d\n",
    irq, x, y, digitalRead(T_DO)
  );

  delay(200);
}
