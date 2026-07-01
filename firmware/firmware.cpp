#include <Arduino.h>
#include "driver/touch_pad.h"
#include "driver/ledc.h"

// =======================
// Configuration Constants
// =======================
constexpr int TOUCH_LOGO_PIN = 14;
constexpr int PIEZO_PIN_A = 26;
constexpr int PIEZO_PIN_B = 17;
constexpr int CPU_FREQ_MHZ = 80;          // Lower frequency to save power
constexpr uint32_t SLEEP_TIMEOUT_MS = 15000;
constexpr uint32_t DEBOUNCE_DELAY_MS = 50;

// Piano keys (C4 to C5)
constexpr int TOUCH_KEYS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
constexpr float PIANO_FREQS[] = {
  261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99,
  392.00, 415.30, 440.00, 466.16, 493.88, 523.25
};

constexpr int NUM_KEYS = sizeof(TOUCH_KEYS) / sizeof(TOUCH_KEYS[0]);

// =======================
// State Variables
// =======================
uint32_t lastTouchTime = 0;
int currentKey = -1;

// =======================
// Utility Functions
// =======================
void playTone(float freq) {
  if (freq <= 0) {
    ledcWriteTone(0, 0);
    return;
  }
  ledcWriteTone(0, freq);
}

int getActiveKey() {
  int activeKeys[NUM_KEYS];
  int activeCount = 0;

  for (int i = 0; i < NUM_KEYS; i++) {
    if (touchRead(TOUCH_KEYS[i]) < 40) { // Threshold, may need tuning
      activeKeys[activeCount++] = i;
    }
  }

  if (activeCount == 0) return -1;

  // Choose center key if multiple touched
  int centerIdx = activeCount / 2;
  return activeKeys[centerIdx];
}

void goToDeepSleep() {
  playTone(0);
  touchSleepWakeUpEnable((touch_pad_t)TOUCH_LOGO_PIN, 40);
  esp_deep_sleep_start();
}

// =======================
// Setup
// =======================
void setup() {
  // Lower CPU frequency
  setCpuFrequencyMhz(CPU_FREQ_MHZ);

  Serial.begin(115200);

  // Touch init
  touch_pad_init();
  for (int i = 0; i < NUM_KEYS; i++) {
    touchAttachInterrupt(TOUCH_KEYS[i], nullptr, 40);
  }
  touchAttachInterrupt(TOUCH_LOGO_PIN, nullptr, 40);

  // Piezo output (using PWM on one pin)
  ledcSetup(0, 2000, 10);
  ledcAttachPin(PIEZO_PIN_A, 0);

  lastTouchTime = millis();
}

// =======================
// Main Loop
// =======================
void loop() {
  int newKey = getActiveKey();

  if (newKey != -1 && newKey != currentKey) {
    delay(DEBOUNCE_DELAY_MS);
    if (getActiveKey() == newKey) {
      playTone(PIANO_FREQS[newKey]);
      currentKey = newKey;
      lastTouchTime = millis();
    }
  } else if (newKey == -1 && currentKey != -1) {
    playTone(0);
    currentKey = -1;
  }

  if (millis() - lastTouchTime > SLEEP_TIMEOUT_MS) {
    goToDeepSleep();
  }
}
