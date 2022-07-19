/*  USB cable tester
 * Send a level high or low on one side of the cable and read from the other side.
 * Then do the same the other way.
 * Display result on the serial monitor.
 * Only testing for continuity. No performance test.
 * Connector 1
 * VBUS --> D5
 * D+   --> D6
 * D-   --> D7
 * GND  --> D8
 * 
 * Connector 2
 * VBUS --> D9
 * D+   --> D10
 * D-   --> D11
 * GND  --> D12
 * 
 * All 8 lines connected to a 4.7k Ohms resistor
 * Common of the resistors connected to D4
 *
 * - All lines set as input
 * - one line set as output
 * - set output HIGH
 * - pull the resistor network LOW
 * - read lines and mark those that are HIGH by adding 1 in a 4x4 array
 * - set output LOW
 * - pull the resistor network HIGH
 * - read lines and mark those that are LOW by adding 1 in the array
 * - repeat for every line
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "usb-cable.h"  // this is the splash screen

// uncomment following line for debug
//#define debug

const uint8_t version = 1;
const uint8_t release = 0;

Adafruit_SSD1306 display(128, 64, &Wire, -1); // reset pin set to -1 to inhibit reset support

const uint8_t pinlist[8] = {5, 6, 7, 8, 9, 10, 11, 12};   // Arduino pins for the 2 USB adaptors
uint8_t pinPull = 4;                                      // Pin the common of the resistor network is connected to
uint8_t cableState[4][4];                                 // Table holding the interconnections

// constant used to draw the interconnections
const char* pinName[4]={"VBUS "," D-  "," D+  ","GND  "}; // Signal names
const uint8_t xVersion = 51;                              // X location of version string
const uint8_t yVersion = 45;                              // Y location of version string
const uint8_t firsLine = 10;                              // Y location of first line of text
const uint8_t yStep = 14;                                 // Step between text lines
const uint8_t xStep = 10;                                 // X step for interconnection drawing
const uint8_t firstCol = 24;                              // Start of interconnection drawing
const uint8_t lastCol = 103;                              // End of interconnection drawing
const uint8_t xID = 8;                                    // X location of IDs
const uint8_t yTexte[4] = {firsLine, firsLine + yStep, firsLine + 2 * yStep, firsLine + 3 * yStep};   // Y location of text lines
const uint8_t yLines[4] = {firsLine + 4, firsLine + yStep + 4, firsLine + 2 * yStep + 4, firsLine + 3 * yStep + 4}; // Y location of interconnection lines
const uint8_t xDown[3] = {firstCol + xStep,firstCol + 2 * xStep,firstCol + 3 *xStep};                               // X position of vertical lines connecting to lines that are under the current one
const uint8_t xUp[3] = {lastCol - xStep,lastCol - 2 * xStep,lastCol - 3 *xStep};                                    // X position of vertical lines connecting to lines that are over the current one

// clearTable
// Clear the interconnection table
void clearTable(void){
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      cableState[i][j] = 0;
    }
  }
}

// drawIDs
// Draw ID of the connectors and pin names on screen
void drawIDs(){
  display.setTextColor(WHITE);
  display.setCursor(xID,0);
  display.print("1");
  display.setCursor(xID + 1,0);
  display.print("1");
  display.setCursor(lastCol + xID,0);
  display.print("2");
  display.setCursor(lastCol + xID + 1,0);
  display.print("2");
  for (uint8_t usb1 = 0; usb1 < 4; usb1++){
    display.setCursor(0,yTexte[usb1]);
    display.print(pinName[usb1]);
    display.setCursor(lastCol + 1,yTexte[usb1]);
    display.print(pinName[usb1]);
  }
}

// Draw the interconnection table on the OLED
void drawTable(void){
  display.clearDisplay();
  drawIDs();
  for (uint8_t usb1 = 0; usb1 < 4; usb1++) {      // for each source
    for (uint8_t usb2 = 0; usb2 < 4; usb2++) {    // for each destination
      uint8_t state = cableState[usb1][usb2];     // read connection state
      if (state != 0){                            // if any
        if (usb1 == usb2){                        // if same number @ both ends draw a horizontal line
          display.drawFastHLine(firstCol, yLines[usb1], lastCol - firstCol, WHITE);
        }
        if (usb1 < usb2){                         // if connected to a higher pin number
          display.drawFastHLine(firstCol, yLines[usb1], xDown[usb2 - 1] - firstCol, WHITE);
          display.drawFastVLine(xDown[usb2 - 1], yLines[usb1], yLines[usb2] - yLines[usb1], WHITE);
          display.drawFastHLine(xDown[usb2 - 1], yLines[usb2], lastCol - xDown[usb2 - 1], WHITE);
          display.drawCircle(xDown[usb2 - 1], yLines[usb2], 2, WHITE);
        }
        if (usb1 > usb2){                         // if connected to a lower pin number
          display.drawFastHLine(firstCol, yLines[usb1], xUp[usb2 - 1] - firstCol, WHITE);
          display.drawFastVLine(xUp[usb2 - 1], yLines[usb2], yLines[usb1] - yLines[usb2], WHITE);
          display.drawFastHLine(xUp[usb2 - 1], yLines[usb2], lastCol - xUp[usb2 - 1], WHITE);
          display.drawCircle(xUp[usb2 - 1], yLines[usb2], 2, WHITE);
        }
      }
    }
  }
  display.display();
}

// display over Serial the connections detected during the scan
void displayTable(void){
  const char* stateStr[4] = {"  _| ","  ~~ ","  ?? "};
  Serial.print("      ");
  for(uint8_t i = 0; i< 4; i++){
    Serial.print(pinName[i]);
  }
  Serial.println();
  for (uint8_t i = 0; i < 4; i++) {
    Serial.print(pinName[i]);
    for (uint8_t j = 0; j < 4; j++) {
      int state = 4;
      switch (cableState[i][j]) {
        case 3:
        case 12:
        case 15:
          state = 0; // connected
          break;
        case 0:
          state = 1;
          break;
        default:
          state = 2;
          break;
      }
      Serial.print(stateStr[state]);
    }
    Serial.println("");
  }
  Serial.println("");
}

/*
    scanlines

  Scan the lines from one connector while they are tied HIGH or LOW from the other
*/
void scanLines(void){
  // drive from connector 1 and read from connector 2
  for (uint8_t usb1 = 0; usb1 < 4; usb1++) {
    pinMode(pinlist[usb1], OUTPUT);                      // set pin usb1 on connector 1 as ouput
    for (uint8_t usb2 = 0; usb2 < 4; usb2++) {           // scan connector 2 lines

      // Output HIGH, lines pulled LOW
      digitalWrite(pinlist[usb1], HIGH);                 // set pin usb1 on connector 1 HIGH
      digitalWrite(pinPull, LOW);                        // pull all the lines to a weak LOW
      delay(2);                                          // wait for the lines to stabilize
      int level = digitalRead(pinlist[usb2 + 4]);        // read line usb2 on connector 2
      cableState[usb1][usb2] |=  (level == HIGH) ? 1 : 0;// if same as input set bit 0

      // Output LOW, lines pulled HIGH
      digitalWrite(pinlist[usb1], LOW);                  // set pin usb1 on connector 1 LOW
      digitalWrite(pinPull, HIGH);                       // pull all the lines to a weak HIGH
      delay(2);                                          // wait for the lines to stabilize
      level = digitalRead(pinlist[usb2 + 4]);            // read line usb2 on connector 2
      cableState[usb1][usb2] |=  (level == LOW) ? 2 : 0; // if same as input set bit 1
    }
    pinMode(pinlist[usb1], INPUT);                       // set pin usb1 on connector 1 as input
  }

}


void setup() {
  char buffer[5];
#ifdef debug
  Serial.begin(115200);
#endif

  for (uint8_t i = 0; i < 8; i++) {
    pinMode(pinlist[i], INPUT); // all I/Os as input without pullup so HiZ
  }
  pinMode(pinPull,OUTPUT);
  digitalWrite(pinPull, HIGH);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_EXTERNALVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)
 
  display.clearDisplay();   // clears the screen
  display.drawBitmap(0, 0, usbcable_data, usbcable_width, usbcable_height,1); // download splash screen
  display.setCursor(xVersion, yVersion);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  sprintf(buffer, "V%1d.%1d", version, release);
  display.print(buffer);
  display.display();        // show splashscreen
  delay(3000);              // delay so user can contemplate the spash screen
  display.setTextSize(1);
  display.clearDisplay();   // clears the screen
  display.display();

}


void loop() {
  clearTable();      // clear connection table
  
  scanLines();       // scan the lines and fill the conection table

#ifdef debug
  displayTable();    // display the table on the terminal
#endif
  drawTable();       // display the table on the OLED display

#ifdef debug
  delay(1600);       // give time to read the display
#endif

}
