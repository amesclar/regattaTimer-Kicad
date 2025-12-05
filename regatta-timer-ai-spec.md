# Sailing Regatta Timer - Technical Specification

**Purpose:** This specification defines the requirements for an Arduino program that runs fixed-duration regatta countdown sequences, triggered by button presses, with real-time logging and display updates.

**Architecture:** The code uses a **blocking (delay-based)** approach to ensure precise, sequential execution of the timer and buzzer events, eliminating the complexity of non-blocking state machines.

---

## 💻 Code and Logging Requirements

### Core Logic
* **Execution Model**: **Blocking**. Once a sequence starts, it runs to completion (countdown loop using `delay()`) and ignores further button input until finished.
* **Logging**: All events must be output to Serial in **JUnit XML format**.
* **Data Storage**: Sequence schedules must be stored using **`PROGMEM`** for memory optimization.

### Event Logging (XML Format)
The log attributes for the sequence name (`whichtest`) and time must be consistent across all event types.

| Event Type | `whichtest` Value | `elapsed` Value | XML Structure (Example) |
| :--- | :--- | :--- | :--- |
| **StartEvent** | "1min", "2min", "3min", "5min" | 0 | `<testcase classname="StartEvent" whichtest="1min" elapsed="0" type="Start"/>` |
| **BuzzerEvent** | "1min", "2min", "3min", "5min" | Current elapsed time (s) | `<testcase classname="BuzzerEvent" whichtest="1min" elapsed="30" type="Buzzer" longcount="0" shortcount="3"/>` |
| **EndEvent** | "1min", "2min", "3min", "5min" | Total duration (s) | `<testcase classname="EndEvent" whichtest="1min" elapsed="60" type="End"/>` |

---

## ⚙️ Hardware Components

### Microcontroller and Serial
* **Platform**: Arduino UNO (or compatible).
* **Baud Rate**: **9600**.

### Display
* **Type**: TM1637 4-digit 7-segment display (requires `TM1637Display.h` library).
* **CLK Pin**: **3**
* **DIO Pin**: **5**
* **Format**: MM:SS (minutes:seconds with colon separator).
    * Colon Bit Mask: `0b01000000`.
    * Format Calculation: `minutes * 100 + seconds`.
* **Brightness**: Maximum (**0x0f**).

### Buzzer
* **Pin**: **4**
* **Type**: Active buzzer.

### Input Buttons
| Button Label | Pin | Pin Mode | Required External Component |
| :--- | :--- | :--- | :--- |
| **1min** | **2** | `INPUT` | External Pull-Down Resistor |
| **2min** | **7** | `INPUT` | External Pull-Down Resistor |
| **3min** | **8** | `INPUT` | External Pull-Down Resistor |
| **5min** | **12** | `INPUT` | External Pull-Down Resistor |

* **Behavior**: High signal (`HIGH`) indicates button press.
* **Debounce**: **200ms** software debounce applied only during button check (`checkButtons`).

---

## ⏰ Timing and Sequences

### Buzzer Timing
| Parameter | Value (ms) |
| :--- | :--- |
| **Long Buzz Duration** | **400** |
| **Short Buzz Duration** | **150** |
| **Gap Between Buzzes** | **150** |
| **Timer Interval (Step)** | **1000** |

### Timer Sequences

#### 1-Minute Sequence (60 seconds)
| Elapsed (s) | Long Buzzes | Short Buzzes |
|-------------|-------------|--------------|
| 0 | 1 | 0 |
| 30 | 0 | 3 |
| 40 | 0 | 2 |
| 50 | 0 | 1 |
| 55 | 0 | 1 |
| 56 | 0 | 1 |
| 57 | 0 | 1 |
| 58 | 0 | 1 |
| 59 | 0 | 1 |
| 60 | 1 | 0 |

#### 2-Minute Sequence (120 seconds)
| Elapsed (s) | Long Buzzes | Short Buzzes |
|-------------|-------------|--------------|
| 0 | 2 | 0 |
| 30 | 1 | 3 |
| 60 | 1 | 0 |
| 90 | 0 | 3 |
| 100 | 0 | 2 |
| 110 | 0 | 1 |
| 115 | 0 | 1 |
| 116 | 0 | 1 |
| 117 | 0 | 1 |
| 118 | 0 | 1 |
| 119 | 0 | 1 |
| 120 | 1 | 0 |

#### 3-Minute Sequence (180 seconds)
| Elapsed (s) | Long Buzzes | Short Buzzes |
|-------------|-------------|--------------|
| 0 | 3 | 0 |
| 60 | 2 | 0 |
| 90 | 1 | 3 |
| 120 | 1 | 0 |
| 150 | 0 | 3 |
| 160 | 0 | 2 |
| 170 | 0 | 1 |
| 175 | 0 | 1 |
| 176 | 0 | 1 |
| 177 | 0 | 1 |
| 178 | 0 | 1 |
| 179 | 0 | 1 |
| 180 | 1 | 0 |

#### 5-Minute Sequence (300 seconds)
| Elapsed (s) | Long Buzzes | Short Buzzes |
|-------------|-------------|--------------|
| 0 | 1 | 0 |
| 60 | 1 | 0 |
| 240 | 1 | 0 |
| 300 | 1 | 0 |

---

## 💾 Implementation Notes

### Data Structure
```cpp
struct BuzzEvent {
    int seconds;    // Elapsed time (s) when to trigger
    int longCount;  // Number of long buzzes
    int shortCount; // Number of short buzzes
};
