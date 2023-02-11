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

#define E_Pin 12
#define R_W 11
#define R_S 10
#define D0 2
#define D1 3
#define D2 4
#define D3 5
#define D4 6
#define D5 7
#define D6 8
#define D7 9

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

void init1() {
  pinMode(R_W, OUTPUT);
  pinMode(R_S, OUTPUT);
  pinMode(E_Pin, OUTPUT);
  for (int i = 0; i < 8; i++) {
    pinMode(DATA_PINS[i], OUTPUT);
  }
  digitalWrite(E_Pin, LOW);
  delay(300);

  switch (mode) {
    case 4:
      sendCommand(0b101001);  //Enable 4-Bit Mode
      delay(5);
      break;
    case 8:
      sendCommand(0b111001);  //Enable 8-Bit Mode
      delay(5);
      break;
  }
  sendCommand(0x08);  //Display OFF
  delay(2);
  sendCommand(0x01);  //Clear Display
  delay(2);
  sendCommand(0b110);  //Entry Mode set
  delay(2);
  sendCommand(0x02);  //Return Home
  delay(2);
  sendCommand(0b1110);  //Display ON
  delay(2);
}

bool isEven(int x) {
  return x % 2 == 0;
}

bool isOdd(int x) {
  return x % 2 == 1;
}

void setCursor(int row, int col) {
  int positionCode = (row / 2 * 20) + col;
  if (isOdd(row)) {
    positionCode |= 0b01000000;
  }
  int command = positionCode | 0b10000000;
  sendCommand(command);
}

/*****************************************************
*           Setup Function, to run once              *
*****************************************************/

void setup() {

  init1();
}

/*****************************************************
*           Loop Function, to run repeatedly         *
*****************************************************/

static char lineBuffer[20];
static int bufferPosition = 0;

void loop() {
  // delay(100);
  // Serial.begin(9600);
  setCursor(0, 9);
  printChar('A');
  delay(200);
  printChar('B');
  delay(200);
  printChar('C');
  delay(200);
  printChar('D');
  delay(200);
  setCursor(1, 9);
  delay(200);
  printChar('A');
  delay(200);
  printChar('B');
  delay(200);
  printChar('C');
  delay(200);
  printChar('D');
  setCursor(2, 9);
  delay(200);
  printChar('A');
  delay(200);
  printChar('B');
  delay(200);
  printChar('C');
  delay(200);
  printChar('D');
  clear_screen();
  delay(200);
  // while (true) {
  //   if (!Serial.available()) {
  //     continue;
  //   }

  //   char nextChar = Serial.read();
  //   lineBuffer[bufferPosition] = nextChar;
  //   if (bufferPosition != 19) {
  //     bufferPosition++;
  //     continue;
  //   }

  //   bufferPosition = 0;
  //   for (int i = 0; i < 20; i++) {
  //     data(lineBuffer[i]);
  //   }
  // }

  // if (Serial.available()) {
  //   // wait a bit for the entire message to arrive
  //   delay(100);
  //   // clear the screen
  //   clear_screen();
  //   // read all the available characters
  //   while (Serial.available() > 0) {
  //     // display each character to the LCD
  //     data(Serial.read());
  //   }
  // }
  // disp1();
  // delay(2500);
  // disp2();
  // delay(2500);
}