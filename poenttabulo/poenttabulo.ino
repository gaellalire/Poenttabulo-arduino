#define bit(b) (1ULL << (b))
#if !defined(BOARD_NAME)
#define BOARD_NAME "Arduino"
#endif
#define BLE_CHARACTERISTIC_SIZE 230

#define POINT_COLOR 0xAA0000
#define SET_COLOR 0xAAAA00

#include <ArduinoJson.h>
#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>  // install Adafruit library from library manager
typedef uint64_t segsize_t;
#include <Noiasca_NeopixelDisplay.h>  // download library from: http://werner.rothschopf.net/202005_arduino_neopixel_display_en.htm

const byte ledPin = 2;                 // Which pin on the Arduino is connected to the NeoPixels?

const byte numDigits = 1;              // How many digits (numbers) are available on each display

const byte pixelPerDigit = 48;         // all pixels, including decimal point pixels if available at each digit
const byte smallPixelPerDigit = 32;    // all pixels, including decimal point pixels if available at each digit
const byte oneOnlyPixelPerDigit = 16;  // all pixels, including decimal point pixels if available at each digit

const byte startPixelHome = 0;
const byte endPixelGuest = 256;

const uint16_t ledCount(endPixelGuest);
/*
   Segments are named and orded like this

          SEG_A
   SEG_F         SEG_B
          SEG_G
   SEG_E         SEG_C
          SEG_D          SEG_DP

  in the following constant array you have to define
  which pixels belong to which segment
*/
/*

0 15 16 31
1 14 17 30
2 13 18 29
3 12 19 28
4 11 20 27
5 10 21 26
6  9 22 25
7  8 23 24
*/

const segsize_t smallSegment[8]{
  bit(16) | bit(15) | bit(0),   // SEG_A
  bit(18) | bit(17) | bit(16),  // SEG_B
  bit(20) | bit(19) | bit(18),  // SEG_C
  bit(20) | bit(11) | bit(4),   // SEG_D
  bit(4) | bit(3) | bit(2),     // SEG_E
  bit(2) | bit(1) | bit(0),     // SEG_F
  bit(18) | bit(13) | bit(2),   // SEG_G
  0                             // SEG_DP
};


const segsize_t segment[8]{
  bit(31) | bit(16) | bit(15) | bit(0),   // SEG_A
  bit(31) | bit(30) | bit(29) | bit(28),  // SEG_B
  bit(28) | bit(27) | bit(26) | bit(25),  // SEG_C
  bit(25) | bit(22) | bit(9) | bit(6),    // SEG_D
  bit(6) | bit(5) | bit(4) | bit(3),      // SEG_E
  bit(3) | bit(2) | bit(1) | bit(0),      // SEG_F
  bit(28) | bit(19) | bit(12) | bit(3),   // SEG_G
  0                                       // SEG_DP
};

segsize_t segmentOneOnly[8];

segsize_t segmentOdd[8];

segsize_t segmentOneOnlyOdd[8];

segsize_t smallSegmentOdd[8];

void moveZigzag(const segsize_t* segment, segsize_t* dsegment, int height, int verticalOffset, int horizontalOffset, int padding) {
  for (int s = 0; s < 8; s++) {
    segsize_t n = segment[s];
    segsize_t d = 0;
    for (int i = 0; i < sizeof(segsize_t) * 8; i++) {
      if ((bit(i) & n) != 0) {
        int b;
        if ((i / height) % 2 == 0) {
          b = i + verticalOffset;
        } else {
          b = i - verticalOffset;
        }
        if (horizontalOffset % 2 != 0) {
          b += 2 * (8 - b % 8) - 1 + height * (horizontalOffset - 1);
        } else {
          b += height * horizontalOffset;
        }
        b += padding;
        if (b >= 0 && b < sizeof(segsize_t) * 8) {
          d |= bit(b);
        }
      }
    }
    dsegment[s] = d;
  }
}


Adafruit_NeoPixel strip(ledCount, ledPin, NEO_GRB + NEO_KHZ800);

Noiasca_NeopixelDisplay centHome(strip, segmentOneOnly, numDigits, oneOnlyPixelPerDigit, startPixelHome);      // create display object, handover the name of your strip as first parameter!
Noiasca_NeopixelDisplay tenHome(strip, segment, numDigits, pixelPerDigit, startPixelHome + 2 * 8);             // create display object, handover the name of your strip as first parameter!
Noiasca_NeopixelDisplay unitHome(strip, segmentOdd, numDigits, pixelPerDigit, startPixelHome + 7 * 8);         // create display object, handover the name of your strip as first parameter!
Noiasca_NeopixelDisplay setHome(strip, smallSegment, numDigits, smallPixelPerDigit, startPixelHome + 12 * 8);  // create display object, handover the name of your strip as first parameter!

Noiasca_NeopixelDisplay setGuest(strip, smallSegmentOdd, numDigits, smallPixelPerDigit, endPixelGuest - 15 * 8);       // create display object, handover the name of your strip as first parameter!
Noiasca_NeopixelDisplay centGuest(strip, segmentOneOnlyOdd, numDigits, oneOnlyPixelPerDigit, endPixelGuest - 11 * 8);  // create display object, handover the name of your strip as first parameter!
Noiasca_NeopixelDisplay tenGuest(strip, segmentOdd, numDigits, pixelPerDigit, endPixelGuest - 9 * 8);                  // create display object, handover the name of your strip as first parameter!
Noiasca_NeopixelDisplay unitGuest(strip, segment, numDigits, pixelPerDigit, endPixelGuest - 4 * 8);                    // create display object, handover the name of your strip as first parameter!

BLEService rfpeliloService("1C144F52-3702-4EFF-9970-C3FECAB28072");

BLECharacteristic txCharacteristic("1C144F53-3702-4EFF-9970-C3FECAB28072",
                                   BLEWrite, BLE_CHARACTERISTIC_SIZE);

void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
}

void txCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
  int valueLength = characteristic.valueLength();
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, characteristic.value(), valueLength);
  int homePoint = doc["home"]["point"];
  int homeSet = doc["home"]["set"];
  int guestPoint = doc["guest"]["point"];
  int guestSet = doc["guest"]["set"];


  int firstDigitHomePoint = homePoint % 10;
  int secondDigitHomePoint = (homePoint / 10) % 10;

  int firstDigitGuestPoint = guestPoint % 10;
  int secondDigitGuestPoint = (guestPoint / 10) % 10;


  if (homePoint >= 100) {
    centHome.print("1");
  } else {
    centHome.print(" ");
  }
  tenHome.print(secondDigitHomePoint);
  unitHome.print(firstDigitHomePoint);
  setHome.print(homeSet % 10);

  setGuest.print(guestSet % 10);
  if (guestPoint >= 100) {
    centGuest.print("1");
  } else {
    centGuest.print(" ");
  }
  tenGuest.print(secondDigitGuestPoint);
  unitGuest.print(firstDigitGuestPoint);
}


void setup() {

  Serial.begin(115200);
  while (!Serial) {}
  moveZigzag(segment, segmentOdd, 8, 0, 1, -8);
  moveZigzag(segment, segmentOneOnly, 8, 0, -3, 0);
  moveZigzag(segmentOneOnly, segmentOneOnlyOdd, 8, 0, 1, -8);
  moveZigzag(smallSegment, smallSegmentOdd, 8, 2, 1, -8);

  if (!BLE.begin()) {
    Serial.println("Starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }

  // set the local name that the peripheral advertises
  BLE.setLocalName(BOARD_NAME);
  // set the UUID for the service the peripheral advertises:
  BLE.setAdvertisedService(rfpeliloService);

  // add the characteristics to the service
  rfpeliloService.addCharacteristic(txCharacteristic);

  // add the service
  BLE.addService(rfpeliloService);

  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  txCharacteristic.setEventHandler(BLEWritten, txCharacteristicWritten);

  // start advertising
  if (!BLE.advertise()) {
    Serial.println("Advertise Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }

  strip.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();             // Turn OFF all pixels ASAP
  strip.setBrightness(50);  // Set BRIGHTNESS to about 1/5 (max = 255)
  strip.clear();

  centHome.setColorFont(POINT_COLOR);
  tenHome.setColorFont(POINT_COLOR);
  unitHome.setColorFont(POINT_COLOR);
  setHome.setColorFont(SET_COLOR);

  setGuest.setColorFont(SET_COLOR);
  centGuest.setColorFont(POINT_COLOR);
  tenGuest.setColorFont(POINT_COLOR);
  unitGuest.setColorFont(POINT_COLOR);

  centHome.print("1");
  tenHome.print("0");
  unitHome.print("0");
  setHome.print("0");

  centGuest.print("1");
  tenGuest.print("0");
  unitGuest.print("0");
  setGuest.print("0");
}

void loop() {
  BLE.poll();
}
