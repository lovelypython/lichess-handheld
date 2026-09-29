#include <Arduino.h>

/*
  ESP32-C5 + XPT2046 standalone diagnostic v2

  Touch wiring:
    T_CLK -> GPIO5
    T_DIN -> GPIO7
    T_DO  -> GPIO25
    T_CS  -> GPIO1
    T_IRQ -> GPIO4

  This firmware intentionally does NOT initialize the LCD.
*/

static constexpr int T_CLK = 5;
static constexpr int T_DIN = 7;
static constexpr int T_DO  = 25;
static constexpr int T_CS  = 1;
static constexpr int T_IRQ = 4;

static uint16_t read12(uint8_t command) {
  digitalWrite(T_CS, LOW);

  // 8-bit command, MSB first
  for (int b = 7; b >= 0; --b) {
    digitalWrite(T_DIN, (command >> b) & 1);
    delayMicroseconds(2);
    digitalWrite(T_CLK, HIGH);
    delayMicroseconds(3);
    digitalWrite(T_CLK, LOW);
    delayMicroseconds(3);
  }

  // One extra acquisition clock before sampling the conversion.
  digitalWrite(T_CLK, HIGH);
  delayMicroseconds(3);
  digitalWrite(T_CLK, LOW);
  delayMicroseconds(3);

  uint16_t value = 0;
  for (int i = 0; i < 12; ++i) {
    digitalWrite(T_CLK, HIGH);
    delayMicroseconds(3);

    value <<= 1;
    value |= digitalRead(T_DO) ? 1 : 0;

    digitalWrite(T_CLK, LOW);
    delayMicroseconds(3);
  }

  digitalWrite(T_CS, HIGH);
  return value & 0x0FFF;
}

static void printNumber(uint16_t n) {
  Serial.print((unsigned int)n);
}

void setup() {
  // On ESP32-C5 with Hardware USB CDC, baud is not a real UART baud.
  Serial.begin();

  const uint32_t start = millis();
  while (!Serial && (millis() - start < 4000)) {
    delay(10);
  }

  delay(250);

  Serial.println();
  Serial.println("ASCII_SERIAL_OK_1234567890");
  Serial.println("ESP32-C5 XPT2046 TEST V2");
  Serial.println("If this text is readable, USB serial is working.");
  Serial.println();

  pinMode(T_CLK, OUTPUT);
  pinMode(T_DIN, OUTPUT);
  pinMode(T_DO, INPUT);
  pinMode(T_CS, OUTPUT);
  pinMode(T_IRQ, INPUT_PULLUP);

  digitalWrite(T_CS, HIGH);
  digitalWrite(T_CLK, LOW);
  digitalWrite(T_DIN, LOW);

  Serial.println("Pins:");
  Serial.println("  CLK=5");
  Serial.println("  DIN=7");
  Serial.println("  DO=25");
  Serial.println("  CS=1");
  Serial.println("  IRQ=4");
  Serial.println();
}

void loop() {
  static uint32_t last = 0;
  static uint32_t counter = 0;

  if (millis() - last >= 250) {
    last = millis();
    ++counter;

    const int irqBefore = digitalRead(T_IRQ);
    const uint16_t x = read12(0xD0);
    delayMicroseconds(100);
    const uint16_t y = read12(0x90);
    const int doState = digitalRead(T_DO);

    // Avoid printf here deliberately: plain ASCII Print API only.
    Serial.print("HB=");
    Serial.print(counter);
    Serial.print(" IRQ=");
    Serial.print(irqBefore);
    Serial.print(" X=");
    printNumber(x);
    Serial.print(" Y=");
    printNumber(y);
    Serial.print(" DO=");
    Serial.println(doState);
  }
}
