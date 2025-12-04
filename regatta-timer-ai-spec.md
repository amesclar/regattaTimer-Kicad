# Sailing Regatta Timer - Technical Specification
**Note, fine tuning code iteration is/was required. These are only captured in Gemini version.**

Arduino program that initiates 1min, 2min, 3min or 5min timer/buzzer sequence based on button press. Log entries for sequence start, end and each buzzer activation. Log messages should be JUnit XML format with event name, time in seconds. For start/end messages test sequence and which test should also be included.

The code can be blocking; i.e. once the 1m, 2m, 3m or 5m sequence starts it will run to completion except reset interrupt.

## Hardware Components
- external pull-down resistors for 1min, 2min, 3min, 5min buttons

### Display
- **Type**: TM1637 4-digit 7-segment display
- **CLK Pin**: 3
- **DIO Pin**: 5
- **Format**: MM:SS (minutes:seconds with colon separator)
- **Brightness**: Maximum (0x0f)

### Buzzer
- **Pin**: 4
- **Type**: Active buzzer

### Input Buttons
| Button Label | Pin | Function | Duration |
|--------------|-----|----------| -------- |
| 1min | 2 | Start 1-minute sequence | 60 seconds |
| 2min | 7 | Start 2-minute sequence | 120 seconds |
| 3min | 8 | Start 3-minute sequence | 180 seconds |
| 5min | 12 | Start 5-minute sequence | 300 seconds |

**Pin Mode**: INPUT (not INPUT_PULLUP)
**Behavior**: One sequence per button press, no repeat while running

## Serial Communication
- **Baud Rate**: 9600
- **Format**: XML testcase fragments
- **Purpose**: Event logging

## Buzzer Timing

### Buzzer Durations
| Type | Duration (ms) |
|------|---------------|
| Long | 400 |
| Short | 150 |

**Gap Between Buzzes**: 150ms

## Timer Sequences

### 1-Minute Sequence (60 seconds)
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

### 2-Minute Sequence (120 seconds)
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

### 3-Minute Sequence (180 seconds)
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

### 5-Minute Sequence (300 seconds)
| Elapsed (s) | Long Buzzes | Short Buzzes |
|-------------|-------------|--------------|
| 0 | 1 | 0 |
| 60 | 1 | 0 |
| 240 | 1 | 0 |
| 300 | 1 | 0 |

## Software Architecture

### State Management
- **Start Condition**: timer sequence begins only after a button press is detected
- **Elapsed Time**: Tracked in seconds
- **Sequence Selection**: Stores active sequence (1, 2, 3, or 5)

### Event Logging (XML Format)

#### Start Event
```xml
<testcase classname="StartEvent" testsequence="[1|2|3|5]" elapsed="0" type="Start"/>
```

#### Buzzer Event
```xml
<testcase classname="BuzzerEvent" testsequence="[1|2|3|5]" elapsed="[seconds]" type="Buzzer" longcount="[n]" shortcount="[n]"/>
```

#### End Event
```xml
<testcase classname="EndEvent" testsequence="[1|2|3|5]" elapsed="[total_seconds]" type="End"/>
```

## Behavioral Requirements

### Startup
1. Initialize serial at 9600 baud
2. Set display brightness to maximum
3. Display 00:00
4. Configure all pins

### Button Press (Timer Not Running)
1. Detect button press
2. Load appropriate schedule
3. Set total duration
4. Reset elapsed time to 0
5. Log start event
6. Display countdown time
8. Begin countdown
9. Apply 200ms debounce delay

### During Countdown
1. Update display every second (MM:SS format)
2. Increment elapsed seconds
3. Check schedule for buzzer events at current elapsed time
5. When elapsed equals total: log end event, stop timer, display 00:00

### Buzzer Sequence Execution
1. Queue long buzzes first, then short buzzes
2. Play each buzz for specified duration
3. Wait 150ms between buzzes
4. Continue until all buzzes complete

## Implementation Notes

### Libraries Required
- `TM1637Display.h` for 7-segment display control

### Pin Modes
- All button pins: INPUT (high = pressed)
- Buzzer pin: OUTPUT

### Display Format
- Leading zeros shown for proper MM:SS format
- Colon bit mask: `0b01000000`
- Format calculation: `minutes * 100 + seconds`

### Data Structures
```cpp
struct BuzzEvent {
  int seconds;       // When to trigger
  int longCount;     // Number of long buzzes
  int shortCount;    // Number of short buzzes
};
```

### Memory Optimization
- Use `F()` macro for string literals in serial output
- Static schedule arrays defined at compile time
- Minimal dynamic memory allocation

## Design Principles

1. **Single Sequence**: Only one timer sequence runs at a time
2. **Deterministic**: Exact buzzer timing per specification tables
3. **Testable**: XML output enables automated verification
3. **Production Ready**: Minimal overhead, no test/debug code
