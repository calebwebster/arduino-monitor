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

const unsigned int LCD_WIDTH = 20;
const unsigned int LCD_HEIGHT = 4;
const unsigned int MESSAGE_TYPE_LENGTH = 3;
const unsigned int SCREEN_TEXT_LENGTH = 60;
const unsigned int SCROLL_TEXT_LENGTH = 60;
const unsigned int MESSAGE_LENGTH = MESSAGE_TYPE_LENGTH + 60;

int messageCurrentLength = 0;

char receivedScreen[SCREEN_TEXT_LENGTH + 1] = "                                                            ";
char receivedScrollText[SCROLL_TEXT_LENGTH + LCD_WIDTH + 1] = "                    \n";
char message[MESSAGE_LENGTH + 1] = "";

bool isMessageComplete() {
  return messageCurrentLength == MESSAGE_LENGTH;
}

void clearMessageBuffer() {
  message[0] = '\0';
  messageCurrentLength = 0;
}

void copyScreenFromMessage() {
  for (int i = 0; i < SCREEN_TEXT_LENGTH; i++) {
    receivedScreen[i] = message[MESSAGE_TYPE_LENGTH + i];
  }
  receivedScreen[SCREEN_TEXT_LENGTH] = '\0';
}

void copyScrollTextFromMessage() {
  // Prepend spaces equal to the width of one screen
  for (int i = 0; i < LCD_WIDTH; i++) {
    receivedScrollText[i] = ' ';
  }
  for (int i = 0; i < SCROLL_TEXT_LENGTH; i++) {
    receivedScrollText[LCD_WIDTH + i] = message[MESSAGE_TYPE_LENGTH + i];
  }
  receivedScrollText[SCROLL_TEXT_LENGTH + LCD_WIDTH] = '\0';
}

void getMessageType(char *messageType) {
  for (int i = 0; i < MESSAGE_TYPE_LENGTH; i++) {
    messageType[i] = message[i];
  }
  messageType[MESSAGE_TYPE_LENGTH] = '\0';
  return messageType;
}

bool messageIsScreen(char *messageType) {
  return strcmp(messageType, "SCN") == 0;
}

bool messageIsScrollText(char *messageType) {
  return strcmp(messageType, "SCL") == 0;
}

void emptySerialBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}

bool messageTypeIsValid(char *messageType) {
  return messageIsScreen(messageType) || messageIsScrollText(messageType);
}

void processMessage() {
  char messageType[MESSAGE_TYPE_LENGTH + 1];
  getMessageType(messageType);

  if (!messageTypeIsValid(messageType)) {
    emptySerialBuffer();
    Serial.println("Invalid Message; Serial buffer cleared");
    return;
  }

  if (messageIsScreen(messageType)) {
    copyScreenFromMessage();
    Serial.println("Received Screen");
  } else if (messageIsScrollText(messageType)) {
    copyScrollTextFromMessage();
    Serial.println("Received Scrolltext");
  }
}

uint8_t receiveCharsIntoMessage() {
  uint8_t lastMessageLength = messageCurrentLength;
  while (!isMessageComplete() && Serial.available()) {
    const char received = Serial.read();
    message[messageCurrentLength] = received;
    messageCurrentLength++;
  }
  message[messageCurrentLength] = '\0';
  return messageCurrentLength - lastMessageLength;
}

void printBytesReceived(uint8_t bytesReceived) {
  Serial.print("Received ");
  Serial.print(bytesReceived);
  Serial.println(" bytes");
  Serial.print("Buffer: ");
  Serial.println(message);
}

void receiveSerialData() {
  uint8_t bytesReceived = receiveCharsIntoMessage();
  if (bytesReceived > 0) {
    printBytesReceived(bytesReceived);
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
  setCursor(0, 0);
  printString("Starting...");
  delay(1000);
}

int scrollPosition = 0;
int scrollTextLength = 0;
char currentScrollText[MESSAGE_LENGTH + LCD_WIDTH + 1];

void displayScrollText() {
  setCursor(0, 3);
  for (int i = 0; i < LCD_WIDTH; i++) {
    int scrolledCharIndex = (i + scrollPosition) % scrollTextLength;
    printChar(currentScrollText[scrolledCharIndex]);
  }
  scrollPosition++;
}

void displayScreen() {
  for (int i = 0; i < SCREEN_TEXT_LENGTH; i++) {
    if (i % LCD_WIDTH == 0) {
      int line = i / LCD_WIDTH;
      setCursor(0, line);
    }
    printChar(receivedScreen[i]);
  }
}

void updateScrollText() {
  int i;
  for (i = 0; i < SCROLL_TEXT_LENGTH + LCD_WIDTH; i++) {
    char c = receivedScrollText[i];
    if (c == '\n' || c == '\0') break;
    currentScrollText[i] = c;
  }
  scrollTextLength = i;
  currentScrollText[scrollTextLength] = '\0';
}

void resetScrollPosition() {
  scrollPosition = 0;
}

bool isTextFinishedScrolling() {
  return scrollPosition == scrollTextLength;
}

bool isNewScrollTextReceived() {
  return strlen(receivedScrollText) > 0;
}

void clearReceivedScrollText() {
  receivedScrollText[0] = '\0';
}

void loop() {
  receiveSerialData();
  if (isMessageComplete()) {
    processMessage();
    clearMessageBuffer();
  }
  if (isTextFinishedScrolling()) {
    resetScrollPosition();
    if (isNewScrollTextReceived()) {
      updateScrollText();
      clearReceivedScrollText();
    }
  }
  displayScreen();
  displayScrollText();
}
