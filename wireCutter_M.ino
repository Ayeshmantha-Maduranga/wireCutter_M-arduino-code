/*
   8/19/2021
   Ayeshmantha Maduranga
   Iron wire cutter machine
*/
#include <EEPROM.h>

// pin
#define dataPin  7 // blue wire to 74HC595 pin 14
#define latchPin  6 // green to 74HC595 pin 12
#define clockPin  5 // yellow to 74HC595 pin 11
#define U0  14
#define U1  13
#define encoderPin1  2
#define encoderPin2  3
#define encoderPin3  4

#define BLANK  -10

#define alram  8
#define cut  9
#define ret  10
#define Mrev  11
#define Mfor  12

#define CutDev  102

const int ButRaw[] = { 18, 17, 16, 15 };

//var
int NumRaw[2][10];

volatile int lastEncoded = 0;
volatile long encoderValue = 0;

long lastencoderValue = 0;

int lastMSB = 0;
int lastLSB = 0;

int cont = 0;
bool MotorUse = false;
int wirelen = 0;
int wireQty = 0;
int Pre_wireQty = 0;
int cutDelay = 0;

unsigned long time_now1 = 0;
unsigned long time_now2 = 0;
bool timeShow1 = false;
bool timeShow2 = false;

bool FunMode = false;

int pre_p_But = 0;

void setup() {
  // initialize I/O pins
  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(U0, OUTPUT);
  pinMode(U1, OUTPUT);
  pinMode(ButRaw[0], INPUT_PULLUP);
  pinMode(ButRaw[1], INPUT_PULLUP);
  pinMode(ButRaw[2], INPUT_PULLUP);
  pinMode(ButRaw[3], INPUT_PULLUP);

  pinMode(encoderPin1, INPUT);
  pinMode(encoderPin2, INPUT);

  pinMode(alram, OUTPUT);
  pinMode(cut, OUTPUT);
  pinMode(ret, OUTPUT);
  pinMode(Mrev, OUTPUT);
  pinMode(Mfor, OUTPUT);

  //  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  //  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3)
  attachInterrupt(digitalPinToInterrupt(encoderPin1), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPin2), updateEncoder, CHANGE);

  //  Serial.begin(115200);

  for (int i = 0; i < 10; i++) {
    NumRaw[0][i] = BLANK;
    NumRaw[1][i] = BLANK;
  }

  // writeIntIntoEEPROM(6,200);
  // writeIntIntoEEPROM(4,15);

  // set EEPROM val to disply
  SetNumber(0, 0, 1);
  SetNumber(0, 1, 1);
  wirelen = readIntFromEEPROM(0);
  SetNumber(1, 0, wirelen);
  wireQty = readIntFromEEPROM(2);
  SetNumber(2, 0, wireQty);
  Pre_wireQty = readIntFromEEPROM(4);
  SetNumber(2, 1, Pre_wireQty);
  cutDelay = readIntFromEEPROM(6);
}

void loop() {
  //normal mode encode limiter
  if (encoderValue / CutDev >= 9999) encoderValue = 99990;
  else if (encoderValue / CutDev <= -999) encoderValue = -9990;

  SetNumber(1, 1, encoderValue / CutDev);
  ShowNumber();

  //------------------ relays conrol --------------------
  if (keyPad(1, 3) && MotorUse == false) { //<----- forward
    digitalWrite(Mfor, HIGH);
    MotorUse = true;
  }
  else if (FunMode == false) {
    digitalWrite(Mfor, LOW);
    MotorUse = false;
  }
  if (keyPad(2, 3) && MotorUse == false) { //<----- reverse
    digitalWrite(Mrev, HIGH);
    MotorUse = true;
  }
  else {
    digitalWrite(Mrev, LOW);
    MotorUse = false;
  }
  if (keyPad(5, 2) && MotorUse == false) { //<----- cut
    encoderValue = 0;
    digitalWrite(cut, HIGH);
  }
  else if (FunMode == false) {
    digitalWrite(cut, LOW);
  }

  // ------------ automation mode funtion ----------------
  if (keyPad(0, 3)) //<------ set funtion mode 
  {
    FunMode = true;
  }
  else if (keyPad(4, 3))
  {
    FunMode = false;
    digitalWrite(Mfor, LOW);
  }

  if (FunMode)
  {
    // motor forwerd 
    digitalWrite(Mfor, HIGH);
    if ((encoderValue / CutDev) >= wirelen)
    {
      digitalWrite(cut, HIGH);
      time_now1 = millis();
      encoderValue = 0;
      Pre_wireQty += 1;
      SetNumber(2, 1, Pre_wireQty);
      writeIntIntoEEPROM(4, Pre_wireQty);
    }
    if (millis() > time_now1 + cutDelay)
    {
      time_now1 = millis();
      digitalWrite(cut, LOW);
    }
  }


  // ------------ set cuting delay ---------------------
  if (keyPad(3, 2))
  {
    // clean all desplay
    SetNumber(1, 0, BLANK);
    SetNumber(2, 0, BLANK);
    SetNumber(1, 1, BLANK);
    SetNumber(2, 1, BLANK);

    //set val and t1 mark
    NumRaw[1][4] = -1; // -
    NumRaw[1][5] = -2; // t
    while (1)
    {
      //refresh display
      ShowNumber();

      if (keyPad(6, 0)) { //<------ set val
        //EEPROM set val
        writeIntIntoEEPROM(6, cutDelay);

        // set EEPROM val to disply
        SetNumber(0, 0, 1);
        SetNumber(0, 1, 1);
        SetNumber(1, 0, wirelen);
        SetNumber(2, 0, wireQty);
        SetNumber(2, 1, Pre_wireQty);
        break;
      }
      if (keyPad(5, 1)) //<------ clear
      {
        SetNumber(2, 1, 0);
        cutDelay = 0;
      }
      if (keyPad(4, 2))//<------- cancel
      {
        // set EEPROM val to disply
        SetNumber(0, 0, 1);
        SetNumber(0, 1, 1);
        SetNumber(1, 0, wirelen);
        SetNumber(2, 0, wireQty);
        SetNumber(2, 1, Pre_wireQty);
        cutDelay = readIntFromEEPROM(6);
        break;
      }
      //init number pad
      cutDelay = numberPad(cutDelay);

      //blink
      if (millis() > time_now1 + 500)
      {
        time_now1 = millis();
        if (timeShow1)
        {
          SetNumber(2, 1, cutDelay);
          timeShow1 = false;
        }
        else
        {
          SetNumber(2, 1, BLANK);
          timeShow1 = true;
        }
      }
    }
  }



  //-------------------- set val ------------------------
  if (keyPad(5, 0)) {
    //add val down raw
    SetNumber(0, 1, 1);
    SetNumber(1, 1, wirelen);
    SetNumber(2, 1, wireQty);

    bool setLen = false;
    while (1) {
      //refresh display
      ShowNumber();

      if (keyPad(6, 0)) { //<------ set complet
        Pre_wireQty = 0;
        //EEPROM set val
        writeIntIntoEEPROM(0, wirelen);
        writeIntIntoEEPROM(2, wireQty);
        writeIntIntoEEPROM(4, Pre_wireQty);
        // blink disply

        //remove val down raw
        SetNumber(0, 1, BLANK);
        SetNumber(1, 1, BLANK);
        SetNumber(2, 1, BLANK);

        // set EEPROM val to disply
        SetNumber(0, 0, 1);
        SetNumber(0, 1, 1);
        SetNumber(1, 0, wirelen);
        SetNumber(2, 0, wireQty);
        SetNumber(2, 1, Pre_wireQty);
        break;
      }
      if (keyPad(5, 1)) //<------ clear
      {
        setLen = false;
        SetNumber(1, 1, 0);
        SetNumber(2, 1, 0);
        wirelen = 0;
        wireQty = 0;
      }
      if (keyPad(4, 2))//<------- cancel
      {
        //get val in rom 
        wirelen = readIntFromEEPROM(0);
        wireQty = readIntFromEEPROM(2);
        Pre_wireQty = readIntFromEEPROM(4);
        // set EEPROM val to disply
        SetNumber(0, 0, 1);
        SetNumber(0, 1, 1);
        SetNumber(1, 0, wirelen);
        SetNumber(2, 0, wireQty);
        SetNumber(2, 1, Pre_wireQty);
        break;
      }


      if (setLen == false)
      {
        //blink
        if (millis() > time_now1 + 500)
        {
          time_now1 = millis();
          if (timeShow1)
          {
            SetNumber(1, 1, wirelen);
            timeShow1 = false;
          }
          else
          {
            SetNumber(1, 1, BLANK);
            timeShow1 = true;
          }
        }
        //init number pad
        wirelen = numberPad(wirelen);

        if (keyPad(6, 1) && wirelen != 0) //<---- confirm
        {
          SetNumber(1, 1, wirelen);
          setLen = true;
        }
      }
      else
      {
        //blink
        if (millis() > time_now1 + 500)
        {
          time_now1 = millis();
          if (timeShow1)
          {
            SetNumber(2, 1, wireQty);
            timeShow1 = false;
          }
          else
          {
            SetNumber(2, 1, BLANK);
            timeShow1 = true;
          }
        }
        //init number pad
        wireQty = numberPad(wireQty);
      }
    }
  }
}

int numberPad(int PreVal) {
  if (keyPad(0, 0))//<--1
  {
    if (pre_p_But != 1) {
      if (PreVal == 0) PreVal = 1;
      else PreVal = (PreVal * 10) + 1; //Pressed twice
    }
    pre_p_But = 1;
  }
  else if (keyPad(1, 0))//<--2
  {
    if (pre_p_But != 2) {
      if (PreVal == 0) PreVal = 2;
      else PreVal = (PreVal * 10) + 2; //Pressed twice
    }
    pre_p_But = 2;
  }
  else if (keyPad(2, 0))//<--3
  {
    if (pre_p_But != 3) {
      if (PreVal == 0) PreVal = 3;
      else PreVal = (PreVal * 10) + 3; //Pressed twice
    }
    pre_p_But = 3;
  }
  else if (keyPad(3, 0))//<--4
  {
    if (pre_p_But != 4) {
      if (PreVal == 0) PreVal = 4;
      else PreVal = (PreVal * 10) + 4; //Pressed twice
    }
    pre_p_But = 4;
  }
  else if (keyPad(4, 0))//<--5
  {
    if (pre_p_But != 5) {
      if (PreVal == 0) PreVal = 5;
      else PreVal = (PreVal * 10) + 5; //Pressed twice
    }
    pre_p_But = 5;
  }
  else if (keyPad(0, 1))//<--6
  {
    if (pre_p_But != 6) {
      if (PreVal == 0) PreVal = 6;
      else PreVal = (PreVal * 10) + 6; //Pressed twice
    }
    pre_p_But = 6;
  }
  else if (keyPad(1, 1))//<--7
  {
    if (pre_p_But != 7) {
      if (PreVal == 0) PreVal = 7;
      else PreVal = (PreVal * 10) + 7; //Pressed twice
    }
    pre_p_But = 7;
  }
  else if (keyPad(2, 1))//<--8
  {
    if (pre_p_But != 8) {
      if (PreVal == 0) PreVal = 8;
      else PreVal = (PreVal * 10) + 8; //Pressed twice
    }
    pre_p_But = 8;
  }
  else if (keyPad(3, 1))//<--9
  {
    if (pre_p_But != 9) {
      if (PreVal == 0) PreVal = 9;
      else PreVal = (PreVal * 10) + 9; //Pressed twice
    }
    pre_p_But = 9;
  }
  else if (keyPad(4, 1))//<--0
  {
    if (pre_p_But != 0) {
      if (PreVal == 0) PreVal = 0;
      else PreVal = (PreVal * 10) + 0; //Pressed twice
    }
    pre_p_But = 0;
  }
  return PreVal;
}

void updateEncoder() {
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number
  int sum = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue--;

  //  Serial.println(encoded);
  lastEncoded = encoded; //store this value for next time
}

void SetNumber(int DisplayNumX, int DisplayNumY, int num)
{
  int count = 0;
  bool num_abs = false;
  if (num < 0 && num != -10)
  {
    num = abs(num);
    num_abs = true;
  }

  switch (DisplayNumX)
  {
    // disply 0 ------------------------------------
    case 0:
      if (num > 99) num = 99;// lemit out of range
      if (num < 10) NumRaw[DisplayNumY][0] = BLANK;
      else NumRaw[DisplayNumY][0] = num / 10;
      NumRaw[DisplayNumY][1] = num % 10;
      break;

      // disply 1 ------------------------------------
    case 1:
      if (num > 9999) num = 9999; // lemit out of range
      NumRaw[DisplayNumY][2] = BLANK;
      NumRaw[DisplayNumY][3] = BLANK;
      NumRaw[DisplayNumY][4] = BLANK;
      if (num == 0) NumRaw[DisplayNumY][5] = 0;
      else NumRaw[DisplayNumY][5] = BLANK;

      while (num > 0) //do till num greater than  0
      {
        NumRaw[DisplayNumY][5 - count] = num % 10;  //split last digit from number
        count++;
        num = num / 10;    //divide num by 10. num /= 10 also a valid one
      }
      if (num_abs) NumRaw[DisplayNumY][5 - count] = -1;
      break;

      // disply 2 ------------------------------------
    case 2:
      if (num > 9999) num = 9999; // lemit out of range
      NumRaw[DisplayNumY][6] = BLANK;
      NumRaw[DisplayNumY][7] = BLANK;
      NumRaw[DisplayNumY][8] = BLANK;
      if (num == 0) NumRaw[DisplayNumY][9] = 0;
      else NumRaw[DisplayNumY][9] = BLANK;
      while (num > 0) //do till num greater than  0
      {
        NumRaw[DisplayNumY][9 - count] = num % 10;  //split last digit from number
        count++;
        num = num / 10;    //divide num by 10. num /= 10 also a valid one
      }
      if (num_abs) NumRaw[DisplayNumY][5 - count] = -1;
      break;
  }
}

bool keyPad(int x, int y) {
  digitalWrite(latchPin, LOW);  // prepare shift register for data
  shiftOut(dataPin, clockPin, MSBFIRST, B00000000); // send data
  shiftOut(dataPin, clockPin, MSBFIRST, B00000000); // send data
  shiftOut(dataPin, clockPin, MSBFIRST, ~(1 << x)); // send data
  digitalWrite(latchPin, HIGH); // update outputs


  if (digitalRead(ButRaw[y]))
  {
    if (millis() > time_now2 + 100)
    {
      time_now2 = millis();
      digitalWrite(alram, LOW);
      pre_p_But = -1;
    }
    return false;
  }
  else
  {
    time_now2 = millis();
    digitalWrite(alram, HIGH);
    return true;
  }
}

void ShowNumber() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(latchPin, LOW);  // prepare shift register for data

    if (NumRaw[1][i] != BLANK)shiftOut(dataPin, clockPin, MSBFIRST, myfnNumToBits(NumRaw[1][i])); // send data
    else shiftOut(dataPin, clockPin, MSBFIRST, B00000000); // send data

    if (NumRaw[0][i] != BLANK)shiftOut(dataPin, clockPin, MSBFIRST, myfnNumToBits(NumRaw[0][i])); // send data
    else shiftOut(dataPin, clockPin, MSBFIRST, B00000000); // send data

    if (i < 8) {
      digitalWrite(U0, LOW);
      digitalWrite(U1, LOW);
      shiftOut(dataPin, clockPin, MSBFIRST, ~(1 << i)); // send data
      digitalWrite(latchPin, HIGH); // update display
    }
    else if (i == 8 && (NumRaw[0][8] != BLANK || NumRaw[1][8] != BLANK)) {
      digitalWrite(U1, LOW);
      shiftOut(dataPin, clockPin, MSBFIRST, B11111111); // send data
      digitalWrite(latchPin, HIGH); // update display
      digitalWrite(U0, HIGH);
    }
    else if (i == 9 && (NumRaw[0][9] != BLANK || NumRaw[1][9] != BLANK)) {
      digitalWrite(U0, LOW);
      shiftOut(dataPin, clockPin, MSBFIRST, B11111111); // send data
      digitalWrite(latchPin, HIGH); // update display
      digitalWrite(U1, HIGH);
    }
    else {
      shiftOut(dataPin, clockPin, MSBFIRST, B11111111); // send data
      digitalWrite(latchPin, HIGH); // update display
    }
  }
}

void writeIntIntoEEPROM(int address, int number) {
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address) {
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

byte myfnNumToBits(int someNumber) {
  switch (someNumber) {
    case 0: return B00111111; break;
    case 1: return B00000110; break;
    case 2: return B01011011; break;
    case 3: return B01001111; break;
    case 4: return B01100110; break;
    case 5: return B01101101; break;
    case 6: return B01111101; break;
    case 7: return B00000111; break;
    case 8: return B01111111; break;
    case 9: return B01101111; break;
    case -1: return B01000000; break;
    case -2: return B01111000; break;
    default: B00000000; break;
  }
}
