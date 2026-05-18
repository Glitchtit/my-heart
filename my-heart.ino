#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// --- Configuration ---
constexpr uint8_t  BUTTON_PIN       = 2;          // button between D2 and GND (uses INPUT_PULLUP)
constexpr uint8_t  DFPLAYER_RX_PIN  = 10;         // Arduino RX  <-  DFPlayer TX
constexpr uint8_t  DFPLAYER_TX_PIN  = 11;         // Arduino TX  ->  DFPlayer RX (through 1k resistor)
constexpr uint8_t  VOLUME           = 22;         // 0..30
constexpr uint8_t  TRACK_COUNT      = 10;         // number of MP3 files on the SD card (0001.mp3 ... 000N.mp3)
constexpr uint16_t DEBOUNCE_MS      = 40;

// --- Globals ---
SoftwareSerial dfSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
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

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(9600);
  dfSerial.begin(9600);

  seedRandomFromAnalogNoise();
  refillBag();

  if (!df.begin(dfSerial)) {
    Serial.println(F("DFPlayer init failed. Check wiring and SD card."));
    while (true) { delay(500); }
  }
  df.volume(VOLUME);
  Serial.println(F("Ready. Press the button."));
}

void loop() {
  uint8_t btn = digitalRead(BUTTON_PIN);
  uint32_t now = millis();

  if (btn != lastBtnState && (now - lastBtnEdgeMs) > DEBOUNCE_MS) {
    lastBtnEdgeMs = now;
    if (lastBtnState == HIGH && btn == LOW) {     // falling edge = press
      uint8_t t = nextTrack();
      Serial.print(F("Playing track "));
      Serial.println(t);
      df.play(t);
    }
    lastBtnState = btn;
  }
}
