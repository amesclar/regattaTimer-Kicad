#include <TM1637Display.h>

// Pin definitions
#define CLK 3
#define DIO 5
#define BUZZER_PIN 4
#define BTN_1MIN 2
#define BTN_2MIN 7
#define BTN_3MIN 8
#define BTN_5MIN 12

// Buzzer durations
#define LONG_BUZZ 400
#define SHORT_BUZZ 150

// Display object
TM1637Display display(CLK, DIO);

// Timer state
unsigned long startMillis = 0;
unsigned long elapsedSeconds = 0;
int totalSeconds = 0;
bool timerRunning = false;
unsigned long lastSecondUpdate = 0;
int currentSequence = 0; // 1, 2, 3, or 5 minutes

// Buzzer state (non-blocking)
unsigned long buzzerStartTime = 0;
int buzzerDuration = 0;
bool buzzerActive = false;
int longBuzzCount = 0;
int shortBuzzCount = 0;
int currentBuzzIndex = 0;

// Buzzer schedule structure
struct BuzzEvent {
  int seconds;
  int longCount;
  int shortCount;
};

// Buzzer schedules for each sequence
BuzzEvent schedule1min[] = {
  {0, 1, 0}, {30, 0, 3}, {40, 0, 2}, {50, 0, 1},
  {55, 0, 1}, {56, 0, 1}, {57, 0, 1}, {58, 0, 1},
  {59, 0, 1}, {60, 1, 0}
};
int schedule1minSize = 10;

BuzzEvent schedule2min[] = {
  {0, 2, 0}, {30, 1, 3}, {60, 1, 0}, {90, 0, 3},
  {100, 0, 2}, {110, 0, 1}, {115, 0, 1}, {116, 0, 1},
  {117, 0, 1}, {118, 0, 1}, {119, 0, 1}, {120, 1, 0}
};
int schedule2minSize = 12;

BuzzEvent schedule3min[] = {
  {0, 3, 0}, {60, 2, 0}, {90, 1, 3}, {120, 1, 0},
  {150, 0, 3}, {160, 0, 2}, {170, 0, 1}, {175, 0, 1},
  {176, 0, 1}, {177, 0, 1}, {178, 0, 1}, {179, 0, 1},
  {180, 1, 0}
};
int schedule3minSize = 13;

BuzzEvent schedule5min[] = {
  {0, 1, 0}, {60, 1, 0}, {240, 1, 0}, {300, 1, 0}
};
int schedule5minSize = 4;

// Current schedule
BuzzEvent* currentSchedule = nullptr;
int currentScheduleSize = 0;
int scheduleIndex = 0;

void setup() {
  Serial.begin(9600);

  // Initialize display
  display.setBrightness(0x0f);
  display.showNumberDecEx(0, 0b01000000, true);

  // Initialize pins
  pinMode(BTN_1MIN, INPUT);
  pinMode(BTN_2MIN, INPUT);
  pinMode(BTN_3MIN, INPUT);
  pinMode(BTN_5MIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
  // Check button presses (only when timer not running)
  if (!timerRunning) {
    if (digitalRead(BTN_1MIN) == HIGH) {
      startTimer(60, schedule1min, schedule1minSize, 1);
    } else if (digitalRead(BTN_2MIN) == HIGH) {
      startTimer(120, schedule2min, schedule2minSize, 2);
    } else if (digitalRead(BTN_3MIN) == HIGH) {
      startTimer(180, schedule3min, schedule3minSize, 3);
    } else if (digitalRead(BTN_5MIN) == HIGH) {
      startTimer(300, schedule5min, schedule5minSize, 5);
    }
  }

  // Update timer
  if (timerRunning) {
    unsigned long currentMillis = millis();

    // Update elapsed seconds
    if (currentMillis - lastSecondUpdate >= 1000) {
      lastSecondUpdate = currentMillis;
      elapsedSeconds++;

      // Update display
      int remaining = totalSeconds - elapsedSeconds;
      if (remaining < 0) remaining = 0;
      int minutes = remaining / 60;
      int seconds = remaining % 60;
      display.showNumberDecEx(minutes * 100 + seconds, 0b01000000, true);

      // Check for buzzer events
      checkBuzzerEvents();

      // Check if timer finished
      if (elapsedSeconds >= totalSeconds) {
        endTimer();
      }
    }
  }

  // Handle non-blocking buzzer
  updateBuzzer();
}

void startTimer(int duration, BuzzEvent* schedule, int scheduleSize, int sequence) {
  timerRunning = true;
  totalSeconds = duration;
  elapsedSeconds = 0;
  startMillis = millis();
  lastSecondUpdate = startMillis;
  currentSchedule = schedule;
  currentScheduleSize = scheduleSize;
  scheduleIndex = 0;
  currentSequence = sequence;

  // Display initial time
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  display.showNumberDecEx(minutes * 100 + seconds, 0b01000000, true);

  // Log start event
  Serial.print(F("<testcase classname=\"StartEvent\" testsequence=\""));
  Serial.print(currentSequence);
  Serial.print(F("\" elapsed=\""));
  Serial.print(elapsedSeconds);
  Serial.println(F("\" type=\"Start\"/>"));

  // Check for buzzer event at 0 seconds
  checkBuzzerEvents();

  delay(200); // Debounce
}

void endTimer() {
  timerRunning = false;

  // Log end event
  Serial.print(F("<testcase classname=\"EndEvent\" testsequence=\""));
  Serial.print(currentSequence);
  Serial.print(F("\" elapsed=\""));
  Serial.print(elapsedSeconds);
  Serial.println(F("\" type=\"End\"/>"));

  // Display 00:00
  display.showNumberDecEx(0, 0b01000000, true);
}

void checkBuzzerEvents() {
  // Check if current elapsed seconds matches a scheduled event
  if (scheduleIndex < currentScheduleSize) {
    if (elapsedSeconds == currentSchedule[scheduleIndex].seconds) {
      // Start buzzer sequence
      longBuzzCount = currentSchedule[scheduleIndex].longCount;
      shortBuzzCount = currentSchedule[scheduleIndex].shortCount;
      currentBuzzIndex = 0;

      // Log buzzer event
      Serial.print(F("<testcase classname=\"BuzzerLogic\" testsequence=\""));
      Serial.print(currentSequence);
      Serial.print(F("\" elapsed=\""));
      Serial.print(elapsedSeconds);
      Serial.print(F("\" type=\"Buzzer\" longcount=\""));
      Serial.print(longBuzzCount);
      Serial.print(F("\" shortcount=\""));
      Serial.print(shortBuzzCount);
      Serial.println(F("\"/>"));

      // Start first buzz
      if (longBuzzCount > 0 || shortBuzzCount > 0) {
        startBuzz();
      }

      scheduleIndex++;
    }
  }
}

void startBuzz() {
  // Determine which type of buzz to play
  if (currentBuzzIndex < longBuzzCount) {
    // Play long buzz
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerStartTime = millis();
    buzzerDuration = LONG_BUZZ;
    buzzerActive = true;
  } else if (currentBuzzIndex < longBuzzCount + shortBuzzCount) {
    // Play short buzz
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerStartTime = millis();
    buzzerDuration = SHORT_BUZZ;
    buzzerActive = true;
  }
}

void updateBuzzer() {
  if (buzzerActive) {
    unsigned long currentMillis = millis();

    // Check if buzz duration completed
    if (currentMillis - buzzerStartTime >= buzzerDuration) {
      digitalWrite(BUZZER_PIN, LOW);
      buzzerActive = false;
      currentBuzzIndex++;

      // Wait 150ms between buzzes
      delay(150);

      // Start next buzz if needed
      if (currentBuzzIndex < longBuzzCount + shortBuzzCount) {
        startBuzz();
      }
    }
  }
}
