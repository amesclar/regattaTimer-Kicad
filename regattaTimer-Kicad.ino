/*
 * Sailing Regatta Timer
 * Based on regatta-timer-ai-spec.md
 *
 * Target: Arduino UNO
 * Pins:
 *  - Display CLK: 3
 *  - Display DIO: 5
 *  - Buzzer: 4
 *  - Buttons: 2 (1min), 7 (2min), 8 (3min), 12 (5min)
 *
 * Logging: Serial 9600, JUnit XML format
 */

#include <Arduino.h>
#include <TM1637Display.h>
#include <avr/pgmspace.h>

// --- Configuration ---
#define BAUD_RATE 115200
#define PIN_CLK 3
#define PIN_DIO 5
#define PIN_BUZZER 4
#define PIN_BTN_1MIN 2
#define PIN_BTN_2MIN 7
#define PIN_BTN_3MIN 8
#define PIN_BTN_5MIN 12

#define DISPLAY_BRIGHTNESS 0x0f

#define BUZZ_LONG_MS 400
#define BUZZ_SHORT_MS 150
#define BUZZ_GAP_MS 150
#define TIMER_INTERVAL_MS 1000

// --- Data Structures ---
struct BuzzEvent {
  int seconds;        // Elapsed time (s) when to use this event
  uint8_t longCount;  // Number of long buzzes
  uint8_t shortCount; // Number of short buzzes
};

// --- Sequences (PROGMEM) ---

// 1-Minute Sequence
const BuzzEvent seq1Min[] PROGMEM = {
    {0, 1, 0},  {30, 0, 3}, {40, 0, 2}, {50, 0, 1}, {55, 0, 1},
    {56, 0, 1}, {57, 0, 1}, {58, 0, 1}, {59, 0, 1}, {60, 1, 0}};

// 2-Minute Sequence
const BuzzEvent seq2Min[] PROGMEM = {{0, 2, 0},   {30, 1, 3},  {60, 1, 0},
                                     {90, 0, 3},  {100, 0, 2}, {110, 0, 1},
                                     {115, 0, 1}, {116, 0, 1}, {117, 0, 1},
                                     {118, 0, 1}, {119, 0, 1}, {120, 1, 0}};

// 3-Minute Sequence
const BuzzEvent seq3Min[] PROGMEM = {
    {0, 3, 0},   {60, 2, 0},  {90, 1, 3},  {120, 1, 0}, {150, 0, 3},
    {160, 0, 2}, {170, 0, 1}, {175, 0, 1}, {176, 0, 1}, {177, 0, 1},
    {178, 0, 1}, {179, 0, 1}, {180, 1, 0}};

// 5-Minute Sequence
const BuzzEvent seq5Min[] PROGMEM = {
    {0, 1, 0}, {60, 1, 0}, {240, 1, 0}, {300, 1, 0}};

// --- Globals ---
TM1637Display display(PIN_CLK, PIN_DIO);

// --- Functions ---

void playBuzzes(int longCount, int shortCount, int elapsed,
                const char *testName) {
  // Log BuzzerEvent
  // Time spent here contributes to the second tick, but we log the start of it.
  // Format: <testcase classname="BuzzerEvent" whichtest="1min" elapsed="30"
  // type="Buzzer" longcount="0" shortcount="3"/>

  Serial.print(F("<testcase classname=\"BuzzerEvent\" whichtest=\""));
  Serial.print(testName);
  Serial.print(F("\" elapsed=\""));
  Serial.print(elapsed);
  Serial.print(F("\" type=\"Buzzer\" longcount=\""));
  Serial.print(longCount);
  Serial.print(F("\" shortcount=\""));
  Serial.print(shortCount);
  Serial.println(F("\"/>"));

  // Execute buzzes
  // Pattern: Longs then Shorts (based on table order in spec "Long Buzzes |
  // Short Buzzes") Spec doesn't explicitly define order but "Long ... Short"
  // columns imply grouping.

  for (int i = 0; i < longCount; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(BUZZ_LONG_MS);
    digitalWrite(PIN_BUZZER, LOW);
    if (i < longCount - 1 || shortCount > 0) {
      delay(BUZZ_GAP_MS);
    }
  }

  for (int i = 0; i < shortCount; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(BUZZ_SHORT_MS);
    digitalWrite(PIN_BUZZER, LOW);
    if (i < shortCount - 1) {
      delay(BUZZ_GAP_MS);
    }
  }
}

void updateDisplay(int secondsRemaining) {
  // MM:SS format
  int m = secondsRemaining / 60;
  int s = secondsRemaining % 60;
  // Format: minutes * 100 + seconds
  // Colon bit mask: 0b01000000 (which is 0x40)
  // showNumberDecEx(number, dots, leading_zeros)
  // dots = 0x40 for colon (default in library usually, but spec says
  // 0b01000000)
  display.showNumberDecEx(m * 100 + s, 0b01000000, true);
}

void runSequence(const BuzzEvent *events, int eventCount, int duration,
                 const char *name) {
  // Log StartEvent
  Serial.print(F("<testcase classname=\"StartEvent\" whichtest=\""));
  Serial.print(name);
  Serial.print(F("\" elapsed=\"0\" type=\"Start\"/>\n"));

  unsigned long startMillis = millis();

  for (int elapsed = 0; elapsed <= duration; elapsed++) {
    // Current target start of this second is startMillis + (elapsed * 1000)
    // Wait for it if we are still ahead (unlikely normally, but handles lead-in)
    unsigned long targetStart = startMillis + (unsigned long)elapsed * 1000;
    while (millis() < targetStart) {
      // Small spin wait for absolute precision at start of second
    }

    // Calculate Remaining Time
    int remaining = duration - elapsed;
    updateDisplay(remaining);

    // Check for events
    int lCount = 0;
    int sCount = 0;
    bool doBuzz = false;
    for (int i = 0; i < eventCount; i++) {
      int evSeconds = (int)pgm_read_word(&events[i].seconds);
      if (evSeconds == elapsed) {
        BuzzEvent ev;
        memcpy_P(&ev, &events[i], sizeof(BuzzEvent));
        lCount = ev.longCount;
        sCount = ev.shortCount;
        doBuzz = true;
        break;
      }
    }

    if (elapsed == duration) {
      // Log EndEvent before final buzzer to minimize reported duration drift
      Serial.print(F("<testcase classname=\"EndEvent\" whichtest=\""));
      Serial.print(name);
      Serial.print(F("\" elapsed=\""));
      Serial.print(elapsed);
      Serial.println(F("\" type=\"End\"/>"));
    }

    if (doBuzz) {
      playBuzzes(lCount, sCount, elapsed, name);
    }

    if (elapsed == duration) break;

    // No need for a simple delay here. We will catch up in the NEXT loop iteration
    // using the 'while (millis() < targetStart)' logic for the next 'elapsed'.
  }


  // Clear display or leave 00:00? Spec doesn't say.
  // Usually regatta timers stay at 00:00 or reset.
  // We'll leave it at 00:00 as it was the last update.
}

// Track previous button states for edge detection, initialized to HIGH by
// default
int lastBtn1 = HIGH;
int lastBtn2 = HIGH;
int lastBtn3 = HIGH;
int lastBtn5 = HIGH;

void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_DIO, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  pinMode(PIN_BTN_1MIN, INPUT_PULLUP);
  pinMode(PIN_BTN_2MIN, INPUT_PULLUP);
  pinMode(PIN_BTN_3MIN, INPUT_PULLUP);
  pinMode(PIN_BTN_5MIN, INPUT_PULLUP);

  display.setBrightness(DISPLAY_BRIGHTNESS);
  display.showNumberDecEx(0, 0b01000000, true); // initial 00:00

  // Read initial state so we only trigger on CHANGES (switches from HIGH to
  // LOW) This fixes the auto-start issue if a button is stuck LOW at boot.
  lastBtn1 = digitalRead(PIN_BTN_1MIN);
  lastBtn2 = digitalRead(PIN_BTN_2MIN);
  lastBtn3 = digitalRead(PIN_BTN_3MIN);
  lastBtn5 = digitalRead(PIN_BTN_5MIN);

  Serial.println(F("--- SUT Boot ---"));
  Serial.print(F("1m="));
  Serial.print(digitalRead(PIN_BTN_1MIN));
  Serial.print(F(" 2m="));
  Serial.print(digitalRead(PIN_BTN_2MIN));
  Serial.print(F(" 3m="));
  Serial.print(digitalRead(PIN_BTN_3MIN));
  Serial.print(F(" 5m="));
  Serial.println(digitalRead(PIN_BTN_5MIN));
  Serial.println(F("Ready for buttons..."));
}

void loop() {
  // Read current states
  int btn1 = digitalRead(PIN_BTN_1MIN);
  int btn2 = digitalRead(PIN_BTN_2MIN);
  int btn3 = digitalRead(PIN_BTN_3MIN);
  int btn5 = digitalRead(PIN_BTN_5MIN);

  // Check for Press (HIGH to LOW transition)
  // 1 Minute Button
  if (lastBtn1 == HIGH && btn1 == LOW) {
    delay(50);
    if (digitalRead(PIN_BTN_1MIN) == LOW) {
      runSequence(seq1Min, sizeof(seq1Min) / sizeof(BuzzEvent), 60, "1min");
      // Update states after blocking sequence
      lastBtn1 = digitalRead(PIN_BTN_1MIN);
      lastBtn2 = digitalRead(PIN_BTN_2MIN);
      lastBtn3 = digitalRead(PIN_BTN_3MIN);
      lastBtn5 = digitalRead(PIN_BTN_5MIN);
      return;
    }
  }

  // 2 Minute Button
  else if (lastBtn2 == HIGH && btn2 == LOW) {
    delay(50);
    if (digitalRead(PIN_BTN_2MIN) == LOW) {
      runSequence(seq2Min, sizeof(seq2Min) / sizeof(BuzzEvent), 120, "2min");
      lastBtn1 = digitalRead(PIN_BTN_1MIN);
      lastBtn2 = digitalRead(PIN_BTN_2MIN);
      lastBtn3 = digitalRead(PIN_BTN_3MIN);
      lastBtn5 = digitalRead(PIN_BTN_5MIN);
      return;
    }
  }

  // 3 Minute Button
  else if (lastBtn3 == HIGH && btn3 == LOW) {
    delay(50);
    if (digitalRead(PIN_BTN_3MIN) == LOW) {
      runSequence(seq3Min, sizeof(seq3Min) / sizeof(BuzzEvent), 180, "3min");
      lastBtn1 = digitalRead(PIN_BTN_1MIN);
      lastBtn2 = digitalRead(PIN_BTN_2MIN);
      lastBtn3 = digitalRead(PIN_BTN_3MIN);
      lastBtn5 = digitalRead(PIN_BTN_5MIN);
      return;
    }
  }

  // 5 Minute Button
  else if (lastBtn5 == HIGH && btn5 == LOW) {
    delay(50);
    if (digitalRead(PIN_BTN_5MIN) == LOW) {
      runSequence(seq5Min, sizeof(seq5Min) / sizeof(BuzzEvent), 300, "5min");
      lastBtn1 = digitalRead(PIN_BTN_1MIN);
      lastBtn2 = digitalRead(PIN_BTN_2MIN);
      lastBtn3 = digitalRead(PIN_BTN_3MIN);
      lastBtn5 = digitalRead(PIN_BTN_5MIN);
      return;
    }
  }

  // Update last states
  lastBtn1 = btn1;
  lastBtn2 = btn2;

  lastBtn3 = btn3;
  lastBtn5 = btn5;
  delay(10); // Small loop delay
}
