#include "SevSeg.h"
SevSeg sevseg; //Instantiate a seven segment object
// https://github.com/DeanIsMe/SevSeg

unsigned long previousMillis = 0;
int count = 1000;

// set up variables for buttons
const int buttonPin_l = 19;
const int buttonPin_c = 20;
const int buttonPin_r = 21;
bool buttonState_l = LOW;
bool lastButtonState_l = LOW;
unsigned long lastDebounceTime_l = 0;
bool buttonState_c = LOW;
bool lastButtonState_c = LOW;
unsigned long lastDebounceTime_c = 0;
bool buttonState_r = LOW;
bool lastButtonState_r = LOW;
unsigned long lastDebounceTime_r = 0;

const int ledPin = 6;  // high-power LED pin

void setup() {
  // set up 7seg display parameters
  byte numDigits = 4;
  // pin assignment specific to our PCB board layout
  byte digitPins[] = {5, 9, 10, 15};
  byte segmentPins[] = {7, 14, 17, 3, 2, 8, 16, 4};
  bool resistorsOnSegments = false; // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_ANODE; // See README.md for options
  bool updateWithDelays = false; // Default 'false' is Recommended
  bool leadingZeros = false; // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = false; // Use 'true' if your decimal point doesn't exist or isn't connected. Then, you only need to specify 7 segmentPins[]
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
  updateWithDelays, leadingZeros, disableDecPoint);

  pinMode(ledPin, OUTPUT);
}

// Function to check for button press with debouncing
bool checkButton(int buttonPin, bool &buttonState, bool &lastButtonState, unsigned long &lastDebounceTime, unsigned long debounceDelay = 50) {
  int reading = digitalRead(buttonPin);

  // If the button state has changed (due to noise or press)
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  // Check if the debounce time has passed
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the button state has changed and the debounce time has passed
    if (reading != buttonState) {
      buttonState = reading;

      // Only return true if the button is pressed
      if (buttonState == HIGH) {
        return true;
      }
    }
  }

  // Save the reading as the last state for the next loop
  lastButtonState = reading;

  // Return false if no button press is detected
  return false;
}

void loop() {
  digitalWrite(ledPin, HIGH);
  unsigned long currentMillis = millis();
  sevseg.refreshDisplay();
  if (currentMillis - previousMillis >= 500){
    // just count down for nowas example
    previousMillis = currentMillis;
    count--;
    sevseg.setNumber(count, 0);
  }


  // read the button output
  // note: buttonDown is true if the button was pressed down in this step, buttonState is whether it is currently pressed
  bool buttonDown_l = checkButton(buttonPin_l, buttonState_l, lastButtonState_l, lastDebounceTime_l);
  bool buttonDown_c = checkButton(buttonPin_c, buttonState_c, lastButtonState_c, lastDebounceTime_c);
  bool buttonDown_r = checkButton(buttonPin_r, buttonState_r, lastButtonState_r, lastDebounceTime_r);

}

