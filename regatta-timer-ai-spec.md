<!-- TOC START min:1 max:3 link:true asterisk:false update:true -->
- [Sailing Regatta Timer - Technical Specification](#sailing-regatta-timer---technical-specification)
  - [Hardware Components](#hardware-components)
    - [Display](#display)
    - [Buzzer](#buzzer)
    - [Input Buttons](#input-buttons)
  - [Serial Communication](#serial-communication)
  - [Buzzer Timing](#buzzer-timing)
    - [Buzzer Durations](#buzzer-durations)
  - [Timer Sequences](#timer-sequences)
    - [1-Minute Sequence (60 seconds)](#1-minute-sequence-60-seconds)
    - [2-Minute Sequence (120 seconds)](#2-minute-sequence-120-seconds)
    - [3-Minute Sequence (180 seconds)](#3-minute-sequence-180-seconds)
    - [5-Minute Sequence (300 seconds)](#5-minute-sequence-300-seconds)
  - [Software Architecture](#software-architecture)
    - [State Management](#state-management)
    - [Non-Blocking Design](#non-blocking-design)
    - [Event Logging (XML Format)](#event-logging-xml-format)
  - [Behavioral Requirements](#behavioral-requirements)
    - [Startup](#startup)
    - [Button Press (Timer Not Running)](#button-press-timer-not-running)
    - [During Countdown](#during-countdown)
    - [Buzzer Sequence Execution](#buzzer-sequence-execution)
  - [Implementation Notes](#implementation-notes)
    - [Libraries Required](#libraries-required)
    - [Pin Modes](#pin-modes)
    - [Display Format](#display-format)
    - [Data Structures](#data-structures)
    - [Memory Optimization](#memory-optimization)
  - [Test Verification](#test-verification)
  - [Design Principles](#design-principles)
<!-- TOC END -->


# Sailing Regatta Timer - Technical Specification

## Hardware Components

### Display
- **Type**: TM1637 4-digit 7-segment display
- **CLK Pin**: 3
- **DIO Pin**: 5
- **Format**: MM:SS (minutes:seconds with colon separator)
- **Brightness**: Maximum (0x0f)

### Buzzer
- **Pin**: 4
- **Type**: Active buzzer
- **Implementation**: Non-blocking state machine

### Input Buttons
| Button Label | Pin | Function |
|--------------|-----|----------|
| 1min | 2 | Start 1-minute sequence |
| 2min | 7 | Start 2-minute sequence |
| 3min | 8 | Start 3-minute sequence |
| 5min | 12 | Start 5-minute sequence |

**Pin Mode**: INPUT (not INPUT_PULLUP)
**Behavior**: One sequence per button press, no repeat while running

## Serial Communication
- **Baud Rate**: 9600
- **Format**: XML testcase fragments
- **Purpose**: Event logging and test verification

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
- **Timer State**: Running or stopped
- **Elapsed Time**: Tracked in seconds
- **Buzzer State**: Non-blocking state machine with counts and indices
- **Sequence Selection**: Stores active sequence (1, 2, 3, or 5)

### Non-Blocking Design
- Display updates every 1000ms
- Buzzer uses state machine (no blocking delays during buzz sequences)
- Button debounce: 200ms delay after sequence start

### Event Logging (XML Format)

#### Start Event
```xml
<testcase classname="StartEvent" testsequence="[1|2|3|5]" elapsed="0" type="Start"/>
```

#### Buzzer Event
```xml
<testcase classname="BuzzerLogic" testsequence="[1|2|3|5]" elapsed="[seconds]" type="Buzzer" longcount="[n]" shortcount="[n]"/>
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
7. **Immediately check for 0-second buzzer events**
8. Begin countdown
9. Apply 200ms debounce delay

### During Countdown
1. Update display every second (MM:SS format)
2. Increment elapsed seconds
3. Check schedule for buzzer events at current elapsed time
4. Execute non-blocking buzzer sequences
5. When elapsed equals total: log end event, stop timer, display 00:00

### Buzzer Sequence Execution
1. Queue long buzzes first, then short buzzes
2. Play each buzz for specified duration
3. Wait 150ms between buzzes
4. Continue until all buzzes complete
5. Non-blocking implementation (timer continues)

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

## Test Verification

Example test output for 1-minute sequence:
```xml
<testcase classname="StartEvent" testsequence="1" elapsed="0" type="Start"/>
<testcase classname="BuzzerLogic" testsequence="1" elapsed="0" type="Buzzer" longcount="1" shortcount="0"/>
<testcase classname="BuzzerLogic" testsequence="1" elapsed="30" type="Buzzer" longcount="0" shortcount="3"/>
<testcase classname="BuzzerLogic" testsequence="1" elapsed="40" type="Buzzer" longcount="0" shortcount="2"/>
...
<testcase classname="BuzzerLogic" testsequence="1" elapsed="60" type="Buzzer" longcount="1" shortcount="0"/>
<testcase classname="EndEvent" testsequence="1" elapsed="60" type="End"/>
```

## Design Principles

1. **Non-Blocking**: All delays use millis() comparison except inter-buzz gaps
2. **Single Sequence**: Only one timer sequence runs at a time
3. **Deterministic**: Exact buzzer timing per specification tables
4. **Testable**: XML output enables automated verification
5. **Production Ready**: Minimal overhead, no test/debug code
