// Gemini
// 20251204-093028-
#include <TM1637Display.h>

// ==========================================
// 📌 Pin and Timing Definitions
// ==========================================
#define CLK_PIN 3
#define DIO_PIN 5
#define BUZZER_PIN 4

#define BTN_1MIN 2
#define BTN_2MIN 7
#define BTN_3MIN 8
#define BTN_5MIN 12

// Buzzer Timings (ms)
#define BUZZ_LONG_MS 400
#define BUZZ_SHORT_MS 150
#define BUZZ_GAP_MS 150
#define TIMER_INTERVAL_MS 1000 // Time for each countdown step

// Button Debounce
#define DEBOUNCE_TIME 200 // ms

// ==========================================
// 🧩 Data Structures and Schedules
// ==========================================

struct BuzzEvent {
  int seconds;    // Elapsed time (s) when to trigger
  int longCount;  // Number of long buzzes
  int shortCount; // Number of short buzzes
};

// --- Sequence Schedules (using PROGMEM) ---
const BuzzEvent sequence_1min[] PROGMEM = {
  {0, 1, 0}, {30, 0, 3}, {40, 0, 2}, {50, 0, 1},
  {55, 0, 1}, {56, 0, 1}, {57, 0, 1}, {58, 0, 1},
  {59, 0, 1}, {60, 1, 0}
};
const int size_1min = sizeof(sequence_1min) / sizeof(BuzzEvent);

const BuzzEvent sequence_2min[] PROGMEM = {
  {0, 2, 0}, {30, 1, 3}, {60, 1, 0}, {90, 0, 3},
  {100, 0, 2}, {110, 0, 1}, {115, 0, 1}, {116, 0, 1},
  {117, 0, 1}, {118, 0, 1}, {119, 0, 1}, {120, 1, 0}
};
const int size_2min = sizeof(sequence_2min) / sizeof(BuzzEvent);

const BuzzEvent sequence_3min[] PROGMEM = {
  {0, 3, 0}, {60, 2, 0}, {90, 1, 3}, {120, 1, 0},
  {150, 0, 3}, {160, 0, 2}, {170, 0, 1}, {175, 0, 1},
  {176, 0, 1}, {177, 0, 1}, {178, 0, 1}, {179, 0, 1},
  {180, 1, 0}
};
const int size_3min = sizeof(sequence_3min) / sizeof(BuzzEvent);

const BuzzEvent sequence_5min[] PROGMEM = {
  {0, 1, 0}, {60, 1, 0}, {240, 1, 0}, {300, 1, 0}
};
const int size_5min = sizeof(sequence_5min) / sizeof(BuzzEvent);


// ==========================================
// ⚙️ Global State Variables
// ==========================================
TM1637Display display(CLK_PIN, DIO_PIN);

// Timer State
bool timerRunning = false;
int currentDuration = 0;
int currentSequenceID = 0;
const BuzzEvent* activeSchedule;
int scheduleSize = 0;

// NEW: Variable to hold the descriptive name (e.g., "1min")
const char* currentSequenceName = "unknown";

// Button Debounce
unsigned long lastButtonPress = 0;

// ==========================================
// 🎤 Function Prototypes
// ==========================================
void startTimer(int duration, int sequenceID, const BuzzEvent* schedule, int size);
void executeBuzzSequence(int longCount, int shortCount);
void updateDisplay(int seconds);
void checkButtons(unsigned long currentMillis);
void logBuzzerEvent(int elapsed, int longCount, int shortCount); // MODIFIED
void logStartEvent(const char* sequenceName);
void logEndEvent(); // MODIFIED


// ==========================================
// 🚀 Setup
// ==========================================
void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize Button Pins
  pinMode(BTN_1MIN, INPUT);
  pinMode(BTN_2MIN, INPUT);
  pinMode(BTN_3MIN, INPUT);
  pinMode(BTN_5MIN, INPUT);

  // Stabilize inputs
  digitalRead(BTN_1MIN);
  digitalRead(BTN_2MIN);
  digitalRead(BTN_3MIN);
  digitalRead(BTN_5MIN);

  display.setBrightness(0x0f);
  updateDisplay(0);

  // Initialize debounce timer
  lastButtonPress = millis();

  Serial.println(F("Sailing Regatta Timer Ready"));
}

// ==========================================
// 🔁 Main Loop (SIMPLE BUTTON CHECK ONLY)
// ==========================================
void loop() {
  checkButtons(millis());
}

// ==========================================
// 🛠️ Core Logic Functions
// ==========================================

/**
 * @brief Checks for button presses and starts a timer.
 */
void checkButtons(unsigned long currentMillis) {
  if (timerRunning) {
    return;
  }

  if (currentMillis - lastButtonPress < DEBOUNCE_TIME) {
    return;
  }

  // Read current button states (Active HIGH)
  if (digitalRead(BTN_1MIN) == HIGH) {
    startTimer(60, 1, sequence_1min, size_1min);
    lastButtonPress = currentMillis;
  }
  else if (digitalRead(BTN_2MIN) == HIGH) {
    startTimer(120, 2, sequence_2min, size_2min);
    lastButtonPress = currentMillis;
  }
  else if (digitalRead(BTN_3MIN) == HIGH) {
    startTimer(180, 3, sequence_3min, size_3min);
    lastButtonPress = currentMillis;
  }
  else if (digitalRead(BTN_5MIN) == HIGH) {
    startTimer(300, 5, sequence_5min, size_5min);
    lastButtonPress = currentMillis;
  }
}


/**
 * @brief Initializes and executes the regatta timer sequence (BLOCKING).
 */
void startTimer(int duration, int sequenceID, const BuzzEvent* schedule, int size) {
  // 1. Setup globals
  currentDuration = duration;
  currentSequenceID = sequenceID;
  activeSchedule = schedule;
  scheduleSize = size;
  timerRunning = true;
  int nextEventIndex = 0;

  // 2. CHECK and EXECUTE T=0 BUZZER EVENT (Don't log yet!)
  bool t0_event_occurred = false;
  int t0_long = 0;
  int t0_short = 0;

  if (nextEventIndex < scheduleSize) {
    BuzzEvent event;
    memcpy_P(&event, activeSchedule + nextEventIndex, sizeof(BuzzEvent));

    if (event.seconds == 0) {
        // Execute the physical buzz now.
        executeBuzzSequence(event.longCount, event.shortCount);

        // Save logging data and advance index, but don't log until after StartEvent.
        t0_long = event.longCount;
        t0_short = event.shortCount;
        nextEventIndex++;
        t0_event_occurred = true;
    }
  }

  // 3. Set the descriptive name globally
  switch (duration) {
      case 60: currentSequenceName = "1min"; break;
      case 120: currentSequenceName = "2min"; break;
      case 180: currentSequenceName = "3min"; break;
      case 300: currentSequenceName = "5min"; break;
      default: currentSequenceName = "unknown"; break;
  }

  // 4. Log start (MUST COME FIRST IN THE LOG)
  logStartEvent(currentSequenceName);

  // 5. Log t=0 Buzzer event (MUST COME SECOND IN THE LOG)
  if (t0_event_occurred) {
      logBuzzerEvent(0, t0_long, t0_short);
  }

  updateDisplay(currentDuration);

  // 6. Main Countdown Loop (starts from 1 second elapsed)
  for (int elapsedSeconds = 1; elapsedSeconds <= currentDuration; elapsedSeconds++) {
    unsigned long startTime = millis();

    // Check for buzzer events at the current elapsed second
    if (nextEventIndex < scheduleSize) {
      BuzzEvent event;
      memcpy_P(&event, activeSchedule + nextEventIndex, sizeof(BuzzEvent));

      if (elapsedSeconds == event.seconds) {
        logBuzzerEvent(elapsedSeconds, event.longCount, event.shortCount);
        executeBuzzSequence(event.longCount, event.shortCount);
        nextEventIndex++;
      }
    }

    // Update display and delay
    int remaining = currentDuration - elapsedSeconds;
    updateDisplay(remaining);

    unsigned long timeElapsedInLoop = millis() - startTime;
    if (timeElapsedInLoop < TIMER_INTERVAL_MS) {
      delay(TIMER_INTERVAL_MS - timeElapsedInLoop);
    }
  }

  // 7. Cleanup and Exit
  logEndEvent();
  timerRunning = false;
  updateDisplay(0);
}


/**
 * @brief Executes a buzzer sequence (BLOCKING).
 */
void executeBuzzSequence(int longCount, int shortCount) {
  // A long buzz sequence
  for (int i = 0; i < longCount; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(BUZZ_LONG_MS);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < longCount - 1 || shortCount > 0) {
      delay(BUZZ_GAP_MS); // Gap between buzzes
    }
  }

  // A short buzz sequence
  for (int i = 0; i < shortCount; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(BUZZ_SHORT_MS);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < shortCount - 1) {
      delay(BUZZ_GAP_MS); // Gap between short buzzes
    }
  }

  digitalWrite(BUZZER_PIN, LOW);
}


// ==========================================
// 📊 Utility & Logging Functions
// ==========================================

/**
 * @brief Formats and shows time on the TM1637 display.
 */
void updateDisplay(int secondsRemaining) {
  int minutes = secondsRemaining / 60;
  int seconds = secondsRemaining % 60;
  display.showNumberDecEx(minutes * 100 + seconds, 0b01000000, true);
}

/**
 * @brief Logs the start of the timer sequence.
 */
void logStartEvent(const char* sequenceName) {
  Serial.print(F("<testcase classname=\"StartEvent\" whichtest=\""));
  Serial.print(sequenceName);
  Serial.println(F("\" elapsed=\"0\" type=\"Start\"/>"));
}

/**
 * @brief Logs a buzzer event in XML format (USING GLOBAL NAME).
 */
void logBuzzerEvent(int elapsed, int longCount, int shortCount) {
  Serial.print(F("<testcase classname=\"BuzzerEvent\" whichtest=\"")); // UPDATED ATTRIBUTE
  Serial.print(currentSequenceName); // USING GLOBAL NAME
  Serial.print(F("\" elapsed=\""));
  Serial.print(elapsed);
  Serial.print(F("\" type=\"Buzzer\" longcount=\""));
  Serial.print(longCount);
  Serial.print(F("\" shortcount=\""));
  Serial.print(shortCount);
  Serial.println(F("\"/>"));
}

/**
 * @brief Logs the end of the timer sequence in XML format (USING GLOBAL NAME).
 */
void logEndEvent() {
  Serial.print(F("<testcase classname=\"EndEvent\" whichtest=\"")); // UPDATED ATTRIBUTE
  Serial.print(currentSequenceName); // USING GLOBAL NAME
  Serial.print(F("\" elapsed=\""));
  Serial.print(currentDuration);
  Serial.println(F("\" type=\"End\"/>"));
}
