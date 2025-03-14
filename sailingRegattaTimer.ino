/*
  inspirsed by https://www.instructables.com/Build-your-own-Coutndown-Regatta-Ollie-Box/

*/

// constants won't change. They're used here to set pin numbers:
const bool debug = 1;            // serial out
const int buttonPinBuzzer = 2;   // buzzer on
const int buttonPinTimer2 = 7;   // 3min timer
const int buttonPinTimer3 = 8;   // 3min timer
const int buttonPinTimer5 = 12;  // 5min timer
const int ledPin = 4;           // the number of the LED pin
// const int buzzerPin = 2;              // buzzer pin - output
const int buzzerOnLongMillis = 200;   // The higher the number, the slower the timing.
const int buzzerOnShortMillis = 100;  // The higher the number, the slower the timing.

// variable for reading the pushbutton status
int buttonStateBuzzer = 0;
int buttonStateTimer2 = 0;
int buttonStateTimer3 = 0;
int buttonStateTimer5 = 0;

/*
DURATION  TIME TO START   SOUND SIGNAL
000       3 min           3 long          {0,3,0}
060 sec   2 min           2 long          {60,2,0} {0,2,0}
090 sec   1 min 30 sec    1 long 3 short  {90,1,3} {30,1,3}
120 sec   1 min           1 long          {120,1,0} {60,1,0}
150 sec   30 sec          3 short         {150,0,3} {90,0,3}
160 sec   20 sec          2 short         {160,0,2} {100,0,2}
170 sec   10 sec          1 short         {170,0,1} {110,0,1}
175 sec   Last 5 sec      5@1 short(1 per sec)  {175,0,1} {115,0,1}
176 sec                   1 short         {176,0,1} {116,0,1}
177 sec                   1 short         {177,0,1} {117,0,1}
178 sec                   1 short         {178,0,1} {118,0,1}
179 sec                   1 short         {179,0,1} {119,0,1}
180 sec   START           1 long          {180,1,0} {120,1,0}
*/

// seconds, longCount, shortCount
// 2 minute
unsigned long timer2Array[12][3] = {
  { 0, 2, 0 }, { 30, 1, 3 }, { 60, 1, 0 }, { 90, 0, 3 }, { 100, 0, 2 }, { 110, 0, 1 }, { 115, 0, 1 }, { 116, 0, 1 }, { 117, 0, 1 }, { 118, 0, 1 }, { 119, 0, 1 }, { 120, 1, 0 }
};
// 3 minute
unsigned long timer3Array[13][3] = {
  { 0, 3, 0 }, { 60, 2, 0 }, { 90, 1, 3 }, { 120, 1, 0 }, { 150, 0, 3 }, { 160, 0, 2 }, { 170, 0, 1 }, { 175, 0, 1 }, { 176, 0, 1 }, { 177, 0, 1 }, { 178, 0, 1 }, { 179, 0, 1 }, { 180, 1, 0 }
};
// 5 minute
// 0-class flag up, 1-P flag up, 4-P flag down, 5-class flag down
unsigned long timer5Array[4][3] = {
  { 0 * 60, 1, 0 }, { 1 * 60, 1, 0 }, { 4 * 60, 1, 0 }, { 5 * 60, 1, 0 }
};


void setup() {
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPinBuzzer, INPUT);
  pinMode(buttonPinTimer2, INPUT);
  pinMode(buttonPinTimer3, INPUT);
  pinMode(buttonPinTimer5, INPUT);
  // initialize buzzer pin
  // pinMode(buzzerPin, OUTPUT);
  // initialize serial output
  Serial.begin(9600);
}

void loop() {
  // read the state of the pushbutton value:
  buttonStateBuzzer = digitalRead(buttonPinBuzzer);
  buttonStateTimer2 = digitalRead(buttonPinTimer2);
  buttonStateTimer3 = digitalRead(buttonPinTimer3);
  buttonStateTimer5 = digitalRead(buttonPinTimer5);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonStateBuzzer == HIGH) {
    // buzzer on button
    logMsg("buzzerOn");
    soundBuzzer(buzzerOnLongMillis);

  } else if (buttonStateTimer2 == HIGH) {
    logMsg("2Min-start");
    // rows - zero based row count
    execBuzzer(timer2Array, 11, 3);
    logMsg("2Min-end");

  } else if (buttonStateTimer3 == HIGH) {
    logMsg("3Min-start");
    // rows - zero based row count
    execBuzzer(timer3Array, 12, 3);
    logMsg("3Min-end");

  } else if (buttonStateTimer5 == HIGH) {
    logMsg("5Min-start");
    // rows - zero based row count
    execBuzzer(timer5Array, 3, 3);
    logMsg("5Min-end");

  } else {
    // turn LED off:
    digitalWrite(ledPin, LOW);
  }
}

bool execBuzzer(unsigned long timeArray[][3], int rows, int columns) {
  long testMillis;

  unsigned long currentDelta;
  unsigned long currentMillis;
  unsigned long deltaMillis;
  // unsigned long iCount = 0;
  // unsigned long prevMillis;
  unsigned long startMillis;

  int buzzerLongCount;
  int buzzerShortCount;
  int buzzerDelayMult = 2;
  int i3Count = 0;  // zero is button press to start sequence
  int testValue = 0;

  // Serial.println("testValue,i3Count,iCount,startMillis,currentMillis,testMillis,buzzerOn");
  startMillis = millis();
  // prevMillis = startMillis;


  while (i3Count <= rows) {

    currentMillis = millis();
    testMillis = (currentMillis - startMillis) - (timeArray[i3Count][0] * 1000);

    if (testMillis >= testValue) {

      // long buzzer
      buzzerLongCount = timeArray[i3Count][1];
      // Serial.println(String(buzzerLongCount));

      for (int i = 0; i < buzzerLongCount; i++) {
        logMsg("buzzerLong-" + String(i3Count) + "," + String(timeArray[i3Count][0]) + "," + String(buzzerLongCount) + "," + String(i));
        digitalWrite(ledPin, HIGH);  //light
        soundBuzzer(buzzerOnLongMillis);
        delay(buzzerDelayMult * buzzerOnLongMillis);
      }

      // short buzzer
      buzzerShortCount = timeArray[i3Count][2];

      for (int i = 0; i < buzzerShortCount; i++) {
        logMsg("buzzerShort-" + String(buzzerShortCount) + "," + String(i));
        digitalWrite(ledPin, HIGH);  //light
        soundBuzzer(buzzerOnShortMillis);
        delay(buzzerDelayMult * buzzerOnShortMillis);
      }

      ++i3Count;
      // ++iCount;
      // prevMillis = currentMillis;
      currentMillis = millis();
    }
  }
}

bool soundBuzzer(int timer) {
  digitalWrite(ledPin, HIGH);
  delay(timer);
  digitalWrite(ledPin, LOW);
  return true;
}

bool logMsg(String msg) {
  if (debug) {
    Serial.println(msg);
  }
  return true;
}
