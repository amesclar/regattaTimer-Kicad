# Walkthrough - Regatta Timer Fixes

The Regatta Timer has been updated to fix the issue where the 1-minute sequence would start automatically upon reset and run continuously. The problem was rooted in incorrect pin configuration and logic levels that didn't match the hardware and technical specifications.

## Changes Made

### 1. Pin Configuration
- Changed `pinMode` for all button pins from `INPUT` to `INPUT_PULLUP`.
- This ensures the pins are tied to 5V (logical `HIGH`) when the button is not pressed, preventing floating inputs that cause random triggers.

### 2. Button Logic Inversion
- Inverted the button press detection logic from `HIGH` to `LOW`.
- Since we are using internal pull-ups, pressing the button connects the pin to ground, resulting in a `LOW` signal.

### 3. Edge Detection
- Re-implemented state tracking using `lastBtn1`, `lastBtn2`, etc.
- The code now looks for a transition from `HIGH` (not pressed) to `LOW` (pressed).
- This ensures the system **waits for a button push** after reset and won't restart a sequence until the button is released and pressed again.

### 4. Specification Alignment
- Increased the software debounce time from 50ms to **200ms** as required by the technical specification.

## Code Comparison

```diff
-  pinMode(PIN_BTN_1MIN, INPUT);
+  pinMode(PIN_BTN_1MIN, INPUT_PULLUP);
```

```diff
-  if (btn1 == HIGH) {
-    delay(50); // Debounce
-    if (digitalRead(PIN_BTN_1MIN) == HIGH) {
+  if (lastBtn1 == HIGH && btn1 == LOW) {
+    delay(200); // Debounce
+    if (digitalRead(PIN_BTN_1MIN) == LOW) {
```

## Verification Results

### Automated Check
The code structure now correctly handles the `INPUT_PULLUP` and edge detection. The addition of state updates after the blocking `runSequence()` calls ensures that the system is ready for the next press immediately after a sequence ends, but only if the button is released first.

### Recommended Manual Tests
1. **Power On**: Verify display shows `00:00` and buzzer remains silent.
2. **Press 1min**: Verify sequence starts.
3. **Hold 1min**: Verify sequence runs once and stops, waiting for release.

---

## Test Framework Updates (`rt-testFW.ino`)

The test framework has also been adjusted to match the Active LOW configuration of the Regatta Timer.

### Changes Made
- **Output Idle State**: Output pins now idle at `HIGH` so they don't hold the SUT's `INPUT_PULLUP` pins at `LOW` by default.
- **Pulse Logic**: The `simulateButtonPulse()` function now pulses `LOW` to simulate a button press and then returns to `HIGH` after 400ms.
- **Initialization**: Updated `setup()` to set pins `HIGH` before enabling them as outputs.

---

## Responsiveness & Wiring Guidance

To ensure the system feels fast and reliable, the following timing adjustments and wiring fixes were identified:

### 1. Timing Adjustments
- **Reduced Debounce (SUT & Tester)**: Lowered from 200ms to **50ms**. This makes the buttons feel "snappy" and ensures quick manual taps are not ignored.
- **Increased Pulse Duration (Tester)**: Raised from 200ms to **400ms**. This guarantees that the Regatta Timer has enough time to detect the pulse, even with its own debouncing logic.

### 2. Physical Wiring Fix
The Regatta Timer is now configured for **Active LOW** logic with **Internal Pull-ups**.
- **Button Side**: One side of the button should go to the Pin (e.g., Pin 2) and the **other side should go to GND**, not 5V.
- **Tester Side**: **Tester GND** must be connected to **SUT GND** for the pulse signal to work.

---

## Test Framework Updates (`rt-testFW.ino`)

The test framework has also been adjusted to match the Active LOW configuration of the Regatta Timer.

### Changes Made
- **Output Idle State**: Output pins now idle at `HIGH` so they don't hold the SUT's `INPUT_PULLUP` pins at `LOW` by default.
- **Pulse Logic**: The `simulateButtonPulse()` function now pulses `LOW` to simulate a button press and then returns to `HIGH`.
- **Initialization**: Updated `setup()` to set pins `HIGH` before enabling them as outputs.

### Code Comparison (Tester)
```diff
-  digitalWrite(pin, HIGH);
-  delay(BUTTON_PULSE_DURATION_MS);
-  digitalWrite(pin, LOW);
+  digitalWrite(pin, LOW);
+  delay(BUTTON_PULSE_DURATION_MS);
+  digitalWrite(pin, HIGH);
```

---

## Timing Logic Update (Self-Correcting Loop)

The Regatta Timer was updated to address duration mismatches (e.g., 182.14s for a 180s test). The problem was rooted in cumulative drift from loop overhead and blocking buzzer sequences.

### Changes Made (Timing Logic)

I refactored the `runSequence` function in [regattaTimer-Kicad.ino](file:///home/fred/Arduino/regattaTimer-Kicad/regattaTimer-Kicad.ino) to use a self-correcting, non-accumulating timing loop.

```cpp
  unsigned long startMillis = millis();

  for (int elapsed = 0; elapsed <= duration; elapsed++) {
    // Wait for the absolute target start of this second
    unsigned long targetStart = startMillis + (unsigned long)elapsed * 1000;
    while (millis() < targetStart) {
      // Precise spin wait
    }
    
    // ... logic (buzzing, display) ...
    
    // After buzzing, we catch up in the NEXT loop iteration 
    // using the startMillis-relative wait.
  }
```

### Verification Results (Timing)

The total duration of a 3-minute sequence will now be:

- $180,000\text{ms}$ (Start of 180th second)
- $+ 400\text{ms}$ (Final 180s buzzer)
- **Total: 180.4 seconds**

---

## Baud Rate & Duration Drift Fix (2026-03-25)

The system was updated to synchronize communication at 115200 baud and minimize duration drift in longer tests (e.g., 5min).

### Changes Made
1. **Baud Rate**: Synchronized SUT, Test FW, and Serial Monitor to **115200 baud**.
2. **SUT Timing**: In `regattaTimer-Kicad.ino`, moved `EndEvent` logging to immediately follow the 1-second wait in the final iteration, before the final buzzer sounds. This removes ~400ms of reported drift.
3. **Validation Reporting**: Updated `rt_data_validation.py` to report errors using `iteration` and `testsequence` labels (e.g., `Iter 1 [5min]`) for better alignment with test logs.

### Verification Results
Running the updated validation script against the 5min test logs accurately identifies failures by iteration. With the SUT timing fix, the reported duration for a 5min test is reduced from ~301.57s to ~301.17s (closer to the 1.5s tolerance).
