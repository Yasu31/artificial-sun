#include "SevSeg.h"
SevSeg sevseg; //Instantiate a seven segment object
// https://github.com/DeanIsMe/SevSeg

unsigned long previousMillis = 0;
int count = 0;

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

unsigned long last_increment_time = 0;  // last time the counter was incremented
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
  pinMode(buttonPin_l, INPUT_PULLUP);
  pinMode(buttonPin_c, INPUT_PULLUP);
  pinMode(buttonPin_r, INPUT_PULLUP);
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
      if (buttonState == LOW) {
        return true;
      }
    }
  }

  // Save the reading as the last state for the next loop
  lastButtonState = reading;

  // Return false if no button press is detected
  return false;
}

int convertToBrightness(float value) {
  // convert value between 0~1 to brightness value between 0~255 using an exponential function so that it feels natural to a human
  value = constrain(value, 0, 1);
  if (value == 0)
    return 0;
  float e = 2.71828; // Euler's number
  int brightness = pow(e, 5*value)/pow(e,5) * 255;
  return brightness;
}

void loop() {
  unsigned long currentMillis = millis();
  sevseg.refreshDisplay();

  // read the button output
  // note: buttonDown is true if the button was pressed down in this step, buttonPushed is whether it is currently pressed
  bool buttonDown_l = checkButton(buttonPin_l, buttonState_l, lastButtonState_l, lastDebounceTime_l);
  bool buttonDown_c = checkButton(buttonPin_c, buttonState_c, lastButtonState_c, lastDebounceTime_c);
  bool buttonDown_r = checkButton(buttonPin_r, buttonState_r, lastButtonState_r, lastDebounceTime_r);
  bool buttonPushed_l = buttonState_l == LOW;
  bool buttonPushed_c = buttonState_c == LOW;
  bool buttonPushed_r = buttonState_r == LOW;

  if (buttonPushed_l || buttonPushed_r) {
    // increment the counter at a constant rate if a button is held down
    if (currentMillis - last_increment_time >= 100){
      if (buttonPushed_l)
        count--;
      if (buttonPushed_r)
        count++;
      count = constrain(count, 0, 100);
      last_increment_time = currentMillis;
    }
  }
  analogWrite(ledPin, convertToBrightness(0.01*count));
  sevseg.setNumber(count, 2);
}

