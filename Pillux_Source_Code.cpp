// Copywrite Ismaeel AlAlawi

#include <Wire.h>
#include "Adafruit_TCS34725.h"  // RGB sensor
#include <Adafruit_Sensor.h>    // Adafruit unified sensor library
#include "Adafruit_TSL2591.h"   //
#define TCAADDR 0x70

// Lux sensor
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);

// RGB Sensor with White LED
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

// Function Prototype
void analyzer(float, float*);
void rgb_analyzer(float);
void surface_analyzer(float*);
void compare_results(float, float, float, float);

// LEDs
const int ready1LedPin = 7;
const int processingLedPin = 6;
const int ready2LedPin = 5;
const int goodLedPin = 4;
const int badLedPin = 3;
const int TCSLed = 13;
const int laserPin = 12;

// Buttons
const int buttonApin = 9;

bool button_state = true;

void tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

/*
  Configure Pins and Sensors
*/
void setup() {
    Serial.begin(9600);

    Wire.begin();

    // LED initialization
    pinMode(ready1LedPin, OUTPUT);
    pinMode(ready2LedPin, OUTPUT);
    pinMode(processingLedPin, OUTPUT);
    pinMode(goodLedPin, OUTPUT);
    pinMode(badLedPin, OUTPUT);

    // Button initialization
    pinMode(buttonApin, INPUT_PULLUP);

    // Light emitting device
    pinMode(TCSLed, OUTPUT);
    pinMode(laserPin, OUTPUT);

    // LUX Sensor: Check and Configuration
    tcaselect(1);
    if(!tsl.begin()) {
      Serial.println("No TSL detected.");
      while(1);
    } else {
      tsl.setGain(TSL2591_GAIN_HIGH);
      tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);
    }

     // RGB Sensor: Check and Configuration
    tcaselect(2);
    if(!tcs.begin()) {
      Serial.println("No TCS detected");
      while(1);
    }

    Serial.println("TCS34725 and TSL2591 are ready for measurement: SYSTEM READY");

    digitalWrite(laserPin, HIGH);
    digitalWrite(TCSLed, LOW);

    // Indicate system is ready
    digitalWrite(ready1LedPin, HIGH);

 
}

/*
  Main loop function
  All functions are executed sequentially inside this block
*/
void loop() {
  // RBG and LUX information
  float rgb1[3], rgb2[3];
  float lux1, lux2;

//  while(1) {
//    rgb_analyzer(rgb1);
//    delay(500);
//  }

  // Ready to measure the first pill
  if (digitalRead(buttonApin) == LOW) {
    Serial.println("first pill");
    digitalWrite(ready1LedPin, LOW);
    // blinkLED(processingLedPin);
    while (digitalRead(buttonApin) == HIGH) Serial.println("halt");

    // Analyze the first pill
    analyzer(rgb1, &lux1);

    delay(1000);

    // Ready to measure the second pill
    digitalWrite(ready2LedPin, HIGH);
    Serial.println("second pill");

    while (digitalRead(buttonApin) == HIGH) Serial.println("halt");

    if (digitalRead(buttonApin) == LOW) {
      digitalWrite(ready2LedPin, LOW);
      while (digitalRead(buttonApin) == HIGH) Serial.println("halt");

      // Analyze the second pill
      analyzer(rgb2, &lux2);
    }

    compare_results(rgb1, rgb2, lux1, lux2);
  }
}

/*
  This function executes surface_analyzer() and rgb_analyzer()
  Takes two input argument: 1) array of RBG 2) pointer to lux variable
*/
void analyzer(float rgb[], float* lux) {
  // preferrably blink the processingLedPin, but turning on would be sufficient
  digitalWrite(processingLedPin, HIGH);

  rgb_analyzer(rgb);
  delay(1000);
  surface_analyzer(lux);

  digitalWrite(processingLedPin, LOW);
}

/*
  This function measures the RGB value of the pill
  Takes pointer of RGB array as an input argument
*/
void rgb_analyzer(float rgb[]) {
  // turn on onboard white led
  digitalWrite(TCSLed, HIGH);
  delay(500);
  Serial.print("Laser On, ready to progress");

  tcaselect(2);

  Serial.println("RGB_Analyzer in progress");
  tcs.setInterrupt(false); // turn on LED
  delay(60); // takes 50ms to read value
  tcs.getRGB(&rgb[0], &rgb[1], &rgb[2]); // pass address
  tcs.setInterrupt(true); // turn off LED

  Serial.print(F("R: ")); Serial.println(rgb[0]);
  Serial.print(F("G: ")); Serial.println(rgb[1]);
  Serial.print(F("B: ")); Serial.println(rgb[2]);

  // turn off led
  delay(500);
  digitalWrite(TCSLed, LOW);
}

/*
  This function measures the surface roughness of the pill via LUX value
  Takes pointer to LUX variable as an input argument
*/
void surface_analyzer(float* lux) { // uint16_t
  // turn on Laser
  digitalWrite(laserPin, HIGH);
  delay(500);
  Serial.print("Laser On, ready to progress");

  tcaselect(1);

  Serial.print("Surface_Analyzer in progress: ");
//  tsl.getLuminosity(TSL2591_VISIBLE); // first reading tends to be noisy
//  delay(100);

  tsl.getFullLuminosity();

   uint32_t lum = tsl.getFullLuminosity();
   uint16_t ir, full;
   ir = lum >> 16;
   full = lum & 0xFFFF;

   Serial.print(F("Lux: ")); Serial.println(tsl.calculateLux(full, ir), 6);

   *lux = tsl.calculateLux(full, ir);

//  *lux = tsl.getLuminosity(TSL2591_VISIBLE);
  Serial.print(F("Lux: ")); Serial.println(*lux);

  // turn off Laser
  delay(500);
  digitalWrite(laserPin, LOW);
}

/*
  This function compares the RGB and LUX value for two pills
  If they have almost identical characteristics, they are good pills
  If they have different characteristics, they are bad pills
*/
void compare_results(float rgb1[], float rgb2[], float lux1, float lux2) {
  Serial.print("\n");
  Serial.println("#################");
  Serial.println("LUX TEST RESULTS");
  Serial.print(F("Lux #1: ")); Serial.print(lux1);
  Serial.print(F("| Lux #2: ")); Serial.println(lux2);
  Serial.print("\n");
  Serial.println("RGB TEST RESULTS");
  Serial.print(F("R #1: ")); Serial.print(rgb1[0]);
  Serial.print(F("| R #2: ")); Serial.println(rgb2[0]);
  Serial.print(F("G #1: ")); Serial.print(rgb1[1]);
  Serial.print(F("| G #2: ")); Serial.println(rgb2[1]);
  Serial.print(F("R #1: ")); Serial.print(rgb1[2]);
  Serial.print(F("| R #2: ")); Serial.println(rgb2[2]);
  Serial.println("#################");


  // Derive the deviation of bad pill from good pill
  // if the one of them falls in the deviation, they are identical
  // if the first and second pill is almost identical
  digitalWrite(goodLedPin, HIGH);

  // if the first and second pill is different
  digitalWrite(badLedPin, HIGH);
}
