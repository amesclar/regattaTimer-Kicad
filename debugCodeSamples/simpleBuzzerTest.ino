// ==========================================
// 📌 Pin and Timing Definitions
// ==========================================
// NOTE: Use the pin your buzzer is actually connected to
#define BUZZER_PIN 4

// Buzzer Timings (ms)
#define BUZZ_LONG_MS 400
#define BUZZ_SHORT_MS 150
#define BUZZ_GAP_MS 150

// ==========================================
// ⚙️ Global State Variables (Buzzer State Machine ONLY)
// ==========================================
enum BuzzerState { IDLE, BUZZING_LONG, BUZZING_SHORT, GAP_WAIT };
BuzzerState buzzerState = IDLE;
unsigned long buzzerStateStart = 0;
int longBuzzesRemaining = 0;
int shortBuzzesRemaining = 0;

// ==========================================
// 🎤 Function Prototypes
// ==========================================
void handleBuzzer(unsigned long currentMillis);
void triggerBuzzSequence(int longCount, int shortCount);

// ==========================================
// 🚀 Setup
// ==========================================
void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off

  Serial.println("Attempting to trigger 1 Long Buzz...");

  // Directly call the trigger for the 1-long buzz sequence
  triggerBuzzSequence(1, 0);

  Serial.println("Buzzer state initialized. Checking handleBuzzer...");
}

// ==========================================
// 🔁 Main Loop
// ==========================================
void loop() {
  // Only the non-blocking state machine is running
  handleBuzzer(millis());
}

// ==========================================
// 🛠️ Core Logic Functions
// ==========================================

/**
 * @brief Sets up the buzzer state machine to execute a sequence.
 */
void triggerBuzzSequence(int longCount, int shortCount) {
  longBuzzesRemaining = longCount;
  shortBuzzesRemaining = shortCount;

  // Reset buzzer pin and state before starting a new sequence
  digitalWrite(BUZZER_PIN, LOW);
  buzzerState = IDLE;

  if (longBuzzesRemaining > 0) {
    longBuzzesRemaining--;
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerState = BUZZING_LONG;
    buzzerStateStart = millis();
    Serial.println("Triggered: BUZZING_LONG"); // Diagnostic print
  } else if (shortBuzzesRemaining > 0) {
    shortBuzzesRemaining--;
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerState = BUZZING_SHORT;
    buzzerStateStart = millis();
    Serial.println("Triggered: BUZZING_SHORT"); // Diagnostic print
  } else {
    buzzerState = IDLE;
    Serial.println("Triggered: IDLE (No buzzes)");
  }
}

/**
 * @brief Non-blocking state machine to drive the buzzer.
 */
void handleBuzzer(unsigned long currentMillis) {
  unsigned long duration;

  switch (buzzerState) {
    case IDLE:
      return;

    case BUZZING_LONG:
      duration = BUZZ_LONG_MS;
      goto BUZZING;

    case BUZZING_SHORT:
      duration = BUZZ_SHORT_MS;
      goto BUZZING;

    BUZZING:
      if (currentMillis - buzzerStateStart >= duration) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerState = GAP_WAIT;
        buzzerStateStart = currentMillis;
        Serial.println("State Change: GAP_WAIT (Pin LOW)"); // Diagnostic print
      }
      break;

    case GAP_WAIT:
      if (currentMillis - buzzerStateStart >= BUZZ_GAP_MS) {

        // Sequence completion check
        if (longBuzzesRemaining == 0 && shortBuzzesRemaining == 0) {
            buzzerState = IDLE;
            Serial.println("State Change: IDLE (Sequence Complete)"); // Diagnostic print
            return;
        }

        // Start the next buzz
        if (longBuzzesRemaining > 0) {
          longBuzzesRemaining--;
          digitalWrite(BUZZER_PIN, HIGH);
          buzzerState = BUZZING_LONG;
          buzzerStateStart = currentMillis;
        } else if (shortBuzzesRemaining > 0) {
          shortBuzzesRemaining--;
          digitalWrite(BUZZER_PIN, HIGH);
          buzzerState = BUZZING_SHORT;
          buzzerStateStart = currentMillis;
        }
      }
      break;
  }
}
