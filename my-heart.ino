#include <DFRobotDFPlayerMini.h>

// --- Configuration ---
constexpr uint8_t  BUTTON_PIN  = 2;     // button between D2 and GND (uses INPUT_PULLUP)
constexpr uint8_t  VOLUME      = 22;    // 0..30
constexpr uint8_t  TRACK_COUNT = 53;    // number of MP3 files on the SD card (0001.mp3 ... 000N.mp3)
constexpr uint16_t DEBOUNCE_MS = 40;

// DFPlayer Mini is wired to the Nano's HARDWARE UART:
//   Nano D1 (TX) -> DFPlayer RX  (through a 1k resistor)
//   Nano D0 (RX) <- DFPlayer TX
// Because this UART is shared with the USB-to-serial chip:
//   * You MUST disconnect the DFPlayer's wire from D0 before uploading new firmware,
//     otherwise the upload fails with "not in sync".
//   * The USB serial monitor is unavailable while running. Init failure is signalled
//     by a fast blink on the onboard LED (D13) instead of a serial message.

// --- Globals ---
DFRobotDFPlayerMini df;

uint8_t shuffleBag[TRACK_COUNT];   // shuffled 1..TRACK_COUNT
uint8_t bagPos        = 0;         // next index to play from shuffleBag
uint8_t lastTrack     = 0;         // 0 == none played yet
uint8_t lastBtnState  = HIGH;
uint32_t lastBtnEdgeMs = 0;

void fisherYatesShuffle() {
  for (uint8_t i = TRACK_COUNT - 1; i > 0; --i) {
    uint8_t j = random(i + 1);
    uint8_t tmp = shuffleBag[i];
    shuffleBag[i] = shuffleBag[j];
    shuffleBag[j] = tmp;
  }
}

void refillBag() {
  for (uint8_t i = 0; i < TRACK_COUNT; ++i) shuffleBag[i] = i + 1;
  fisherYatesShuffle();
  // Avoid back-to-back repeat across bag boundaries: if first of new bag equals lastTrack, swap with another slot.
  if (TRACK_COUNT > 1 && shuffleBag[0] == lastTrack) {
    uint8_t swapWith = 1 + random(TRACK_COUNT - 1); // 1..TRACK_COUNT-1
    uint8_t tmp = shuffleBag[0];
    shuffleBag[0] = shuffleBag[swapWith];
    shuffleBag[swapWith] = tmp;
  }
  bagPos = 0;
}

uint8_t nextTrack() {
  if (bagPos >= TRACK_COUNT) refillBag();
  uint8_t t = shuffleBag[bagPos++];
  lastTrack = t;
  return t;
}

void seedRandomFromAnalogNoise() {
  uint32_t seed = 0;
  for (uint8_t i = 0; i < 16; ++i) {
    seed = (seed << 2) ^ analogRead(A0) ^ (analogRead(A1) << 1);
    delay(2);
  }
  randomSeed(seed ? seed : micros());
}

// No serial monitor on this wiring, so blink the onboard LED forever to report a fault.
void blinkForever() {
  pinMode(LED_BUILTIN, OUTPUT);
  while (true) {
    digitalWrite(LED_BUILTIN, HIGH); delay(150);
    digitalWrite(LED_BUILTIN, LOW);  delay(150);
  }
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(9600);          // hardware UART, shared with the DFPlayer Mini

  seedRandomFromAnalogNoise();
  refillBag();

  if (!df.begin(Serial)) {
    blinkForever();            // DFPlayer init failed. Check wiring and SD card.
  }
  df.volume(VOLUME);
}

void loop() {
  uint8_t btn = digitalRead(BUTTON_PIN);
  uint32_t now = millis();

  if (btn != lastBtnState && (now - lastBtnEdgeMs) > DEBOUNCE_MS) {
    lastBtnEdgeMs = now;
    if (lastBtnState == HIGH && btn == LOW) {     // falling edge = press
      df.play(nextTrack());
    }
    lastBtnState = btn;
  }
}
