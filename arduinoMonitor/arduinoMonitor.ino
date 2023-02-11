/*
/Program for writing to NHD-0420DZW series display. 
/This code is written for the Arduino Uno (Microchip ATmega328P) using 4-Bit / 8-Bit 6800 Parallel Interface. 
/
/Newhaven Display invests time and resources providing this open source code,
/Please support Newhaven Display by purchasing products from Newhaven Display! 

* Copyright (c) 2020 Alee Shah - Newhaven Display International, Inc. 
*
* This code is provided as an example only and without any warranty by Newhaven Display. 
* Newhaven Display accepts no responsibility for any issues resulting from its use.
* The developer on the final application incorporating any parts of this 
*  sample code is responsible for ensuring its safe and correct operation
*   and for any consequences resulting from its use. 
* See the GNU General Public License for more details.
* 
* 
*/

/****************************************************
*      PINOUT: Arduino Uno -> Character OLED        *
****************************************************/
// The 8 bit data bus is connected to PORTD 0-7 of the Arduino Uno
// The 4 bit data bus is connected to PORTD 4-7 of the Arduino Uno

#include <ArduinoJson.h>
#include <string.h>

#define R_S 12
#define R_W 11
#define E_Pin 10
#define D0 9  
#define D1 8
#define D2 7
#define D3 6
#define D4 5
#define D5 4
#define D6 3
#define D7 2

uint8_t DATA_PINS[8] = { D0, D1, D2, D3, D4, D5, D6, D7 };

unsigned char mode = 8;  // 4 = 4-Bit parallel
                         // 8 = 8-Bit Parallel

/****************************************************
*                 Text Strings                      *
****************************************************/

unsigned char const text1[] = ("  Newhaven Display  ");
unsigned char const text2[] = ("   International    ");
unsigned char const text3[] = ("   CHARACTER TEST   ");

unsigned char const text4[] = ("   4-Bit Parallel   ");
unsigned char const text5[] = ("   8-Bit Parallel   ");

unsigned char const text6[] = ("ABCDEFGHIJKLMOPQRSTU");
unsigned char const text7[] = ("VWXYZabcdefghijklmno");
unsigned char const text8[] = ("pqrstuvwxyz123456789");
unsigned char const text9[] = (" <(' ')> || <(' ')> ");

/****************************************************
*               Function Commands                   *
****************************************************/

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  4-bit / 8-bit 6800 Parallel Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void pulseEnable()  // command to latch E
{
  digitalWrite(E_Pin, HIGH);
  delay(1);
  digitalWrite(E_Pin, LOW);
  delay(2);
}

void sendByte(unsigned char b) {
  digitalWrite(R_W, LOW);  // Write
  for (int i = 0; i < 8; i++) {
    digitalWrite(DATA_PINS[i], (b >> i) & 0b00000001);
  }
  pulseEnable();
  for (int i = 0; i < 8; i++) {
    digitalWrite(DATA_PINS[i], LOW);
  }

  if (mode == 4) {
  }
}

void sendCommand(unsigned char b) {
  digitalWrite(R_S, LOW);  // Command
  sendByte(b);
}

void printChar(unsigned char b) {
  digitalWrite(R_S, HIGH);  // Character
  sendByte(b);
}

void printString(char *string) {
  for (int i = 0; i < strlen(string); i++) {
    printChar(string[i]);
  }
}

/****************************************************
*                  Display Functions                *
****************************************************/

void clear_screen() {  // clear display
  sendCommand(0x01);
}

void ret_home() {  // Return to home position
  sendCommand(0x02);
}


void disp1() {  // DISPLAY TEXT
  clear_screen();
  ret_home();  // First Line
  for (int i = 0; i < 20; i++) {
    printChar(text1[i]);
  }
  setCursor(1, 0);  // Second Line
  for (int i = 0; i < 20; i++) {
    printChar(text2[i]);
  }
  sendCommand(0x94);  // Third Line
  for (int i = 0; i < 20; i++) {
    printChar(text3[i]);
  }
  sendCommand(0xD4);  // Fourth Line
  for (int i = 0; i < 20; i++) {
    switch (mode) {
      case 4:
        printChar(text4[i]);
        break;
      case 8:
        printChar(text5[i]);
        break;
    }
  }
}


void disp2() {  // DISPLAY TEXT
  clear_screen();
  ret_home();  // First Line
  for (int i = 0; i < 20; i++) {
    printChar(text6[i]);
  }
  sendCommand(0xc0);  // Second Line
  for (int i = 0; i < 20; i++) {
    printChar(text7[i]);
  }
  sendCommand(0x94);  // Third Line
  for (int i = 0; i < 20; i++) {
    printChar(text8[i]);
  }
  sendCommand(0xD4);  // Fourth Line
  for (int i = 0; i < 20; i++) {
    printChar(text9[i]);
  }
}

/****************************************************
*              Initialization Routine               *
****************************************************/

void initializeLCD() {
  pinMode(R_W, OUTPUT);
  pinMode(R_S, OUTPUT);
  pinMode(E_Pin, OUTPUT);
  setDataPinsMode(OUTPUT);

  digitalWrite(E_Pin, LOW);
  if (mode == 4) {
    sendCommand(0b00101000);  //Enable 4-Bit Mode
    delay(5);
  } else {
    sendCommand(0b00111000);  //Enable 8-Bit Mode
    delay(5);
  }
  sendCommand(0x08);  //Display OFF
  delay(2);
  sendCommand(0x01);  //Clear Display
  delay(2);
  sendCommand(0b110);  //Entry Mode set
  delay(2);
  sendCommand(0x02);  //Return Home
  delay(2);
  sendCommand(0b1100);  //Display ON
  delay(2);
}

bool isEven(int x) {
  return x % 2 == 0;
}

bool isOdd(int x) {
  return x % 2 == 1;
}

const unsigned int LCD_WIDTH = 20;
const unsigned int LCD_HEIGHT = 4;
const unsigned int MESSAGE_TYPE_LENGTH = 3;
const unsigned int MESSAGE_LENGTH = 60;
const unsigned int SCREEN_TEXT_LENGTH = MESSAGE_LENGTH;
const unsigned int SCROLL_TEXT_LENGTH = MESSAGE_LENGTH;
const unsigned int MESSAGE_BUFFER_LENGTH = MESSAGE_TYPE_LENGTH + MESSAGE_LENGTH;

bool hasReceivedNewScreen = false;
bool hasReceivedNewScrollText = false;

char receivedScreen[MESSAGE_LENGTH + 1] = "";
char receivedScrollText[MESSAGE_LENGTH + LCD_WIDTH + 1] = "";
char blankScreen[SCREEN_TEXT_LENGTH + 1] = "                                                            ";
char blankScrollText[SCROLL_TEXT_LENGTH + 1] = "                                                            ";
char messageBuffer[MESSAGE_BUFFER_LENGTH + 1] = "";

uint8_t lastBufferSize = 0;

bool isBufferFull() {
  return strlen(messageBuffer) == MESSAGE_BUFFER_LENGTH;
}

bool shouldReceiveCharacter() {
  return !isBufferFull() && Serial.available();
}

void receiveCharacter() {
  const char received = Serial.read();
  strncat(messageBuffer, &received, 1);
}

void emptyBuffer() {
  messageBuffer[0] = '\0';
}

void loadFromBuffer() {
  // Serial.print("Buffer: ");
  // Serial.println(messageBuffer);
  char messageType[MESSAGE_TYPE_LENGTH + 1];
  strncpy(messageType, messageBuffer, MESSAGE_TYPE_LENGTH);
  messageType[MESSAGE_TYPE_LENGTH] = '\0';
  // Serial.print("Message Type: \"");
  // Serial.print(messageType);
  // Serial.println("\"");
  if (strcmp(messageType, "SCN") == 0) {
    strcpy(receivedScreen, &messageBuffer[MESSAGE_TYPE_LENGTH]);
    // Serial.print("Screen: \"");
    // Serial.print(receivedScreen);
    // Serial.println("\"");
    hasReceivedNewScreen = true;
    return;
  }
  if (strcmp(messageType, "SCL") == 0) {
    for (int i = 0; i < LCD_WIDTH; i++) {
      receivedScrollText[i] = ' ';
    }
    strcpy(&receivedScrollText[LCD_WIDTH], &messageBuffer[MESSAGE_TYPE_LENGTH]);
    // Serial.print("Scrolling Message: \"");
    // Serial.print(receivedScrollText);
    // Serial.println("\"");
    hasReceivedNewScrollText = true;
    return;
  }
  // Serial.println("Invalid Message");
}

void processSerialInput() {
  long t1 = millis();
  while (shouldReceiveCharacter()) {
    receiveCharacter();
  }
  // if (strlen(messageBuffer) > lastBufferSize) {
  //   long t2 = millis();
  //   Serial.print("Processed ");
  //   Serial.print(strlen(messageBuffer) - lastBufferSize);
  //   Serial.print(" bytes in ");
  //   Serial.print(t2 - t1);
  //   Serial.println("ms");
  // }
  lastBufferSize = strlen(messageBuffer);
  if (isBufferFull()) {
    loadFromBuffer();
    emptyBuffer();
  }
}

void setDataPinsMode(bool mode) {
  for (int i = 0; i < 8; i++) {
    pinMode(DATA_PINS[i], mode);
  }
}

uint8_t getDataPinsValue() {
  setDataPinsMode(INPUT);
  int value = 0b00000000;
  for (int i = 0; i < 8; i++) {
    int pinValue = digitalRead(DATA_PINS[i]);
    value |= pinValue << i;
  }
  setDataPinsMode(OUTPUT);
  return value;
}

char getCharAtCursor() {
  digitalWrite(R_S, HIGH);
  digitalWrite(R_W, HIGH);
  digitalWrite(E_Pin, HIGH);
  char character = getDataPinsValue();
  digitalWrite(E_Pin, LOW);
  return character;
}

void shiftDisplayLeft() {
  sendCommand(0b00011000);
}

void shiftDisplayRight() {
  sendCommand(0b00011100);
}

void setCursor(uint8_t x, uint8_t y) {
  int ddramAddress = (y / 2 * 20) + x;
  bool yParity = y % 2;
  ddramAddress |= (0b01000000 * yParity);  // Add an offset of 2^6 for odd-numbered rows.
  int command = ddramAddress | 0b10000000;
  sendCommand(command);
}

uint8_t getDDRAMAddress() {
  digitalWrite(R_S, LOW);
  digitalWrite(R_W, HIGH);
  digitalWrite(E_Pin, HIGH);
  uint8_t dataPinsValue = getDataPinsValue();
  digitalWrite(E_Pin, LOW);
  uint8_t ddramAddress = dataPinsValue & 0b01111111;  // 2^8 bit is the busy flag; remove it.
  return ddramAddress;
}

uint8_t getCursorX() {
  /*
  n = 0   - (64 * (n >= 64)) =  0 % 20 = 0
  n = 65  - (64 * (n >= 64)) =  1 % 20 = 1
  n = 39  - (64 * (n >= 64)) = 39 % 20 = 19
  n = 102 - (64 * (n >= 64)) = 38 % 20 = 18
  */
  uint8_t ddramAddress = getDDRAMAddress();
  ddramAddress &= 0b11011111;  // 2^6 bit is an offset added to odd-numbered rows; remove it.
  uint8_t cursorX = ddramAddress % LCD_WIDTH;
  return cursorX;
}

uint8_t getCursorY() {
  /*
  n = 0-19   / 20 = 0 * 2 = 0 + (n >= 64) = 0
  n = 64-83  / 20 = 0 * 2 = 0 + (n >= 64) = 1
  n = 20-39  / 20 = 1 * 2 = 2 + (n >= 64) = 2 
  n = 84-103 / 20 = 1 * 2 = 2 + (n >= 64) = 3
  */
  uint8_t ddramAddress = getDDRAMAddress();
  bool parity = ddramAddress & 0b00100000;  // 2^6 bit is an offset added to odd-numbered rows; determine parity from this bit.
  ddramAddress &= 0b11011111;               // Remove offset bit.
  uint8_t cursorY = (ddramAddress / 20) * 2 + parity;
  return cursorY;
}

bool isLCDBusy() {
  digitalWrite(R_S, LOW);
  digitalWrite(R_W, HIGH);
  digitalWrite(E_Pin, HIGH);
  uint8_t dataPinsValue = getDataPinsValue();
  digitalWrite(E_Pin, LOW);
  uint8_t isBusy = dataPinsValue & 0b10000000;  // 2^8 bit is the busy flag; isolate it.
  return isBusy;
}

void setup() {
  initializeLCD();
  Serial.begin(9600);
  Serial.setTimeout(0);
  Serial.println("Starting...");
  setCursor(0, 0);
  printString("Starting...");
  delay(1000);
}

int timesScrolled = 0;
char currentScrollText[MESSAGE_LENGTH + LCD_WIDTH + 1];


void displayScrollText(char *scrollText) {
  int scrollTextLength = strlen(scrollText);
  setCursor(0, 3);
  for (int i = 0; i < LCD_WIDTH; i++) {
    int scrolledCharIndex = (i + timesScrolled) % scrollTextLength;
    printChar(scrollText[scrolledCharIndex]);
  }
  ++timesScrolled %= strlen(scrollText);
}

bool shouldChangeScrollingText() {
  return timesScrolled == 0 && hasReceivedNewScrollText;
}

void changeToNewScrollingText() {
  strcpy(currentScrollText, receivedScrollText);
  hasReceivedNewScrollText = false;
}

void displayScrollText() {
  if (strlen(receivedScrollText) == 0) {
    displayScrollText(blankScrollText);
    timesScrolled = 0;
    return;
  }
  if (shouldChangeScrollingText()) {
    changeToNewScrollingText();
  }
  displayScrollText(currentScrollText);
}

void displayScreen(char *screen) {
  int screenLength = strlen(screen);
  for (int i = 0; i < screenLength; i++) {
    if (i % LCD_WIDTH == 0) {
      int line = i / LCD_WIDTH;
      setCursor(0, line);
    }
    printChar(screen[i]);
  }
}

void displayScreen() {
  if (strlen(receivedScreen) == 0 || hasReceivedNewScreen) {
    displayScreen(blankScreen);
    hasReceivedNewScreen = false;
    return;
  }
  displayScreen(receivedScreen);
}

void loop() {
  // processSerialInput();
  // displayScreen();
  // displayScrollText();
}
