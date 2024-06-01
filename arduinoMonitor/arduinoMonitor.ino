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

const unsigned int LCD_ROWS = 20;
const unsigned int LCD_COLS = 4;

uint8_t DATA_PINS[8] = { D0, D1, D2, D3, D4, D5, D6, D7 };

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
  uint8_t cursorX = ddramAddress % LCD_ROWS;
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

void pulseEnable() {
  digitalWrite(E_Pin, HIGH);
  delay(1);
  digitalWrite(E_Pin, LOW);
  delay(2);
}

void sendByte(uint8_t b) {
  digitalWrite(R_W, LOW);  // Write
  for (int i = 0; i < 8; i++) {
    digitalWrite(DATA_PINS[i], (b >> i) & 0b00000001);
  }
  pulseEnable();
  for (int i = 0; i < 8; i++) {
    digitalWrite(DATA_PINS[i], LOW);
  }
}

void sendCommand(uint8_t b) {
  digitalWrite(R_S, LOW);  // Command
  sendByte(b);
}

void printChar(uint8_t b) {
  digitalWrite(R_S, HIGH);  // Character
  sendByte(b);
}

void printString(char *string) {
  for (int i = 0; i < strlen(string); i++) {
    printChar(string[i]);
  }
}

void clear_screen() {  // clear display
  sendCommand(0x01);
}

void ret_home() {  // Return to home position
  sendCommand(0x02);
}

void initializeLCD() {
  pinMode(R_W, OUTPUT);
  pinMode(R_S, OUTPUT);
  pinMode(E_Pin, OUTPUT);
  setDataPinsMode(OUTPUT);

  digitalWrite(E_Pin, LOW);
  sendCommand(0b00111011);  //Enable 8-Bit Mode
  delay(5);
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

const unsigned int MESSAGE_TYPE_LENGTH = 3;
const unsigned int SCREEN_TEXT_LENGTH = 60;
const unsigned int SCROLL_TEXT_LENGTH = 60;
const unsigned int MESSAGE_LENGTH = 63;

int receivedMessageLength = 0;
int scrollPosition = 0;
int scrollTextLength = 0;

bool hasReceivedFirstScreen = false;
bool hasReceivedFirstScrollText = false;

char message[MESSAGE_LENGTH];
char screen[SCREEN_TEXT_LENGTH];
char nextScrollText[SCROLL_TEXT_LENGTH];
char scrollText[MESSAGE_LENGTH];

bool isMessageComplete() {
  return receivedMessageLength == MESSAGE_LENGTH;
}

void clearMessageBuffer() {
  receivedMessageLength = 0;
}

void copyMessageToScreen() {
  strncpy(screen, &message[MESSAGE_TYPE_LENGTH], sizeof(screen));
}

void copyMessageToScrollText() {
  strncpy(nextScrollText, &message[MESSAGE_TYPE_LENGTH], sizeof(nextScrollText));
}

bool isMessageScrollText() {
  return strncmp(message, "SCL", 3) == 0;
}

bool isMessageScreen() {
  return strncmp(message, "SCN", 3) == 0;
}

void emptySerialBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}

void processMessage() {
  if (isMessageScreen()) {
    copyMessageToScreen();
    hasReceivedFirstScreen = true;
    Serial.println("Received Screen");
  } else if (isMessageScrollText()) {
    copyMessageToScrollText();
    hasReceivedFirstScrollText = true;
    Serial.println("Received Scrolltext");
  } else {
    emptySerialBuffer();
    Serial.println("Invalid Message; Serial buffer cleared");
  }
}

void receiveMessage() {
  uint8_t bytesReceived = 0;
  while (!isMessageComplete() && Serial.available()) {
    const char received = Serial.read();
    message[receivedMessageLength] = received;
    receivedMessageLength++;
    bytesReceived++;
  }
  if (bytesReceived > 0) {
    Serial.print("Received ");
    Serial.print(bytesReceived);
    Serial.print(" bytes");
    Serial.print(" -> ");
    Serial.println(message);
  }
}

void displayScrollText() {
  setCursor(0, 3);
  if (!hasReceivedFirstScrollText) {
    displayEmptyScrollText();
    return;
  }

  for (int i = 0; i < LCD_ROWS; i++) {
    int scrolledCharIndex = (i + scrollPosition) % SCROLL_TEXT_LENGTH;
    printChar(scrollText[scrolledCharIndex]);
  }

  advanceScrollPosition();
}

void displayEmptyScrollText() {
  setCursor(0, 3);
  for (int i = 0; i < LCD_ROWS; i++) {
    printChar(' ');
  }
}

void displayEmptyScreen() {
  setCursor(0, 0);
  for (int i = 0; i < sizeof(screen); i++) {
    if (i % LCD_ROWS == 0) {
      int line = i / LCD_ROWS;
      setCursor(0, line);
    }
    printChar(' ');
  }
}

void displayScreen() {
  setCursor(0, 0);
  for (int i = 0; i < sizeof(screen); i++) {
    if (i % LCD_ROWS == 0) {
      int line = i / LCD_ROWS;
      setCursor(0, line);
    }
    printChar(screen[i]);
  }
}

void copyNextScrollText() {
  strncpy(scrollText, nextScrollText, sizeof(scrollText));
  // int i;
  // for (i = 0; i < SCROLL_TEXT_LENGTH + LCD_ROWS; i++) {
  //   char c = nextScrollText[i];
  //   if (c == '\n' || c == '\0') break;
  //   scrollText[i] = c;
  // }
  // scrollTextLength = i;
  // scrollText[scrollTextLength] = '\0';
}

void advanceScrollPosition() {
  scrollPosition++;
}

void setup() {
  initializeLCD();
  Serial.begin(9600);
  Serial.setTimeout(0);
  setCursor(0, 0);
  printString("Starting...");
  delay(1000);
}

void loop() {
  receiveMessage();
  if (isMessageComplete()) {
    processMessage();
    clearMessageBuffer();
  }
  if (scrollPosition == 0 || scrollPosition == SCROLL_TEXT_LENGTH) {
    copyNextScrollText();
    scrollPosition = 0;
  }
  if (hasReceivedFirstScreen) {
    displayScreen();
  } else {
    displayEmptyScreen();
  }
  displayScrollText();
}
