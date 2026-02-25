// Lab 2 functional analogue for Wokwi (NUCLEO C031C6)
// Variant 5 target in report: 80 MHz, SysTick 10 kHz, output toggling in handler.
// In this simulation, we emulate 10 kHz tick using micros() and toggle output pin every tick.

const int BTN_UP = D2;
const int BTN_DOWN = D3;
const int LED_PINS[8] = {D4, D5, D6, D7, D8, D9, D10, D11};
const int FREQ_OUT_PIN = D12; // connect logic analyzer here

uint8_t counterVal = 0;
bool upPrev = false;
bool downPrev = false;

unsigned long lastTickUs = 0;
const unsigned long TICK_US = 100; // 10 kHz tick period

void writeLeds(uint8_t value) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(LED_PINS[i], (value >> i) & 0x01);
  }
}

void setup() {
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  for (int i = 0; i < 8; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  pinMode(FREQ_OUT_PIN, OUTPUT);
  digitalWrite(FREQ_OUT_PIN, LOW);

  writeLeds(counterVal);
  lastTickUs = micros();
}

void loop() {
  // Emulated SysTick 10 kHz: every 100 us toggle output pin
  unsigned long now = micros();
  if ((unsigned long)(now - lastTickUs) >= TICK_US) {
    lastTickUs += TICK_US;
    digitalWrite(FREQ_OUT_PIN, !digitalRead(FREQ_OUT_PIN));
  }

  bool upNow = (digitalRead(BTN_UP) == LOW);
  bool downNow = (digitalRead(BTN_DOWN) == LOW);

  if (upNow && !upPrev) {
    counterVal++;
    writeLeds(counterVal);
    delay(50);
  }

  if (downNow && !downPrev) {
    counterVal--;
    writeLeds(counterVal);
    delay(50);
  }

  upPrev = upNow;
  downPrev = downNow;
}
