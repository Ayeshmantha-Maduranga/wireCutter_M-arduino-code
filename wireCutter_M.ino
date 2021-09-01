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

  //writeIntIntoEEPROM(0,200);
  // writeIntIntoEEPROM(4,15);

  // set EEPROM val to disply
  SetNumber(0, 0, 1);
  wirelen = readIntFromEEPROM(0);
  SetNumber(1, 0, wirelen);
  wireQty = readIntFromEEPROM(2);
  SetNumber(2, 0, wireQty);
  Pre_wireQty = readIntFromEEPROM(4);
  SetNumber(2, 1, Pre_wireQty);
}

void loop() {
  ShowNumber();
  if (keyPad(0, 0)) {
    cont += 1;
    SetNumber(1, 1, cont);
    digitalWrite(Mfor, HIGH);
    delay(100);
  }

  if (keyPad(1, 0)) {
    cont -= 1;
    SetNumber(1, 1, cont);
    digitalWrite(Mfor, LOW);
    delay(100);
  }

  SetNumber(1, 1, encoderValue);

  //------------------ relays conrol --------------------
  if (keyPad(1, 3) && MotorUse == false) { //<----- forward
    digitalWrite(Mfor, HIGH);
    MotorUse = true;
  }
  else {
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
    digitalWrite(cut, HIGH);
  }
  else {
    digitalWrite(cut, LOW);
  }

  //------------------ set val ----------------------
  if (keyPad(5, 0)) {
    //remove val up raw
    SetNumber(0, 0, BLANK);
    SetNumber(1, 0, BLANK);
    SetNumber(2, 0, BLANK);
    //add val down raw
    SetNumber(0, 1, 1);
    SetNumber(1, 1, wirelen);
    SetNumber(2, 1, wireQty);

    bool setLen = false;
    while (1) {
      //refresh display
      ShowNumber();

      if (keyPad(6, 0)) { //<------ set complet
        //EEPROM set val
        writeIntIntoEEPROM(0, wirelen);
        writeIntIntoEEPROM(2, wireQty);
        // blink disply

        //remove val down raw
        SetNumber(0, 1, BLANK);
        SetNumber(1, 1, BLANK);
        SetNumber(2, 1, BLANK);

        // set EEPROM val to disply
        SetNumber(0, 0, 1);
        wirelen = readIntFromEEPROM(0);
        SetNumber(1, 0, wirelen);
        wireQty = readIntFromEEPROM(2);
        SetNumber(2, 0, wireQty);
        break;
      }
      if (keyPad(5, 1)) { //<------ clear
        setLen = false;
        SetNumber(1, 1, 0);
        SetNumber(2, 1, 0);
      }

      if (setLen == false) {
        //blink

        //init number pad

        if (keyPad(5, 1)) { //<---- set confirm

        }
      }
    }
  }
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
      if(num_abs) NumRaw[DisplayNumY][5 - count] = -1;
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
      if(num_abs) NumRaw[DisplayNumY][5 - count] = -1;
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
    return false;
  else
    return true;
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
    default: B00000000; break;
  }
}
