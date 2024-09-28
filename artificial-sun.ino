#include "SevSeg.h"
SevSeg sevseg; // Instantiate a seven-segment object
// https://github.com/DeanIsMe/SevSeg

// Variables for the timer
unsigned long lastInteractionTime = 0;
bool displayOn = true;
unsigned long remainingTime = 7UL * 60 * 60 * 1000; // 7 hours in milliseconds
bool timerPaused = true;
unsigned long lastBlinkTime = 0;
bool displayBlinkState = true;
unsigned long lastTimerUpdateTime = 0;

// Variables for center button
unsigned long centerButtonPressTime = 0;
bool centerButtonLongPressHandled = false;
const unsigned long LONG_PRESS_DURATION = 1000; // 1 second for long press

// Variables for left button
unsigned long lastIncrementTime_l = 0;

// Variables for right button
unsigned long lastIncrementTime_r = 0;

// Debounce variables
bool buttonState_l = HIGH;
bool lastButtonState_l = HIGH;
unsigned long lastDebounceTime_l = 0;
bool lastButtonPushed_l = false;

bool buttonState_c = HIGH;
bool lastButtonState_c = HIGH;
unsigned long lastDebounceTime_c = 0;
bool lastButtonPushed_c = false;

bool buttonState_r = HIGH;
bool lastButtonState_r = HIGH;
unsigned long lastDebounceTime_r = 0;
bool lastButtonPushed_r = false;

const int buttonPin_l = 19;
const int buttonPin_c = 20;
const int buttonPin_r = 21;
const int ledPin = 6; // High-power LED pin

void setup() {
  byte numDigits = 4;
  byte digitPins[] = {5, 9, 10, 15};
  byte segmentPins[] = {7, 14, 17, 3, 2, 8, 16, 4};
  bool resistorsOnSegments = false; // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_ANODE;
  bool updateWithDelays = false;
  bool leadingZeros = false;
  bool disableDecPoint = false;
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
               updateWithDelays, leadingZeros, disableDecPoint);

  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin_l, INPUT_PULLUP);
  pinMode(buttonPin_c, INPUT_PULLUP);
  pinMode(buttonPin_r, INPUT_PULLUP);

  lastInteractionTime = millis();
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
  bool buttonPushed_l = (buttonState_l == LOW);

  bool buttonDown_c = checkButton(buttonPin_c, buttonState_c, lastButtonState_c, lastDebounceTime_c);
  bool buttonPushed_c = (buttonState_c == LOW);
  bool buttonReleased_c = (!buttonPushed_c && lastButtonPushed_c);

  bool buttonDown_r = checkButton(buttonPin_r, buttonState_r, lastButtonState_r, lastDebounceTime_r);
  bool buttonPushed_r = (buttonState_r == LOW);

  // Handle left button (decrement time)
  if (buttonDown_l) {
    lastInteractionTime = currentMillis;
    displayOn = true;
  }

  if (buttonPushed_l && timerPaused) {
    if ((currentMillis - lastIncrementTime_l) >= 300) {
      remainingTime -= 60UL * 1000; // Decrement by one minute
      lastIncrementTime_l = currentMillis;
    }
    lastInteractionTime = currentMillis;
    displayOn = true;
  }

  lastButtonPushed_l = buttonPushed_l;

  // Handle right button (increment time)
  if (buttonDown_r) {
    lastInteractionTime = currentMillis;
    displayOn = true;
  }

  if (buttonPushed_r && timerPaused) {
    if ((currentMillis - lastIncrementTime_r) >= 300) {
      remainingTime += 60UL * 1000; // Increment by one minute
      // Optional: set a maximum remainingTime if desired
      lastIncrementTime_r = currentMillis;
    }
    lastInteractionTime = currentMillis;
    displayOn = true;
  }

  lastButtonPushed_r = buttonPushed_r;

  // Handle center button (pause/restart and reset)
  if (buttonDown_c) {
    // Button pressed down
    centerButtonPressTime = currentMillis;
    centerButtonLongPressHandled = false;
    lastInteractionTime = currentMillis;
    displayOn = true;
  }

  if (buttonPushed_c) {
    // Button is being held down
    if (!centerButtonLongPressHandled) {
      unsigned long pressDuration = currentMillis - centerButtonPressTime;
      if (pressDuration >= LONG_PRESS_DURATION) {
        // Long press detected
        centerButtonLongPressHandled = true;
        remainingTime = 7UL * 60 * 60 * 1000; // Reset to 7 hours
        timerPaused = true;
        lastInteractionTime = currentMillis;
        displayOn = true;
      }
    }
  }

  if (buttonReleased_c) {
    // Button was released
    unsigned long pressDuration = currentMillis - centerButtonPressTime;
    if (!centerButtonLongPressHandled && pressDuration < LONG_PRESS_DURATION) {
      // Short press detected
      timerPaused = !timerPaused; // Toggle pause/restart
      lastInteractionTime = currentMillis;
      displayOn = true;
    }
    centerButtonPressTime = 0;
  }

  lastButtonPushed_c = buttonPushed_c;

  // Update the remaining time when the timer is running
  if (!timerPaused && (currentMillis - lastTimerUpdateTime >= 1000)) {
    if (remainingTime >= 1000) {
      remainingTime -= 1000;
    } else {
      remainingTime = 0;
      timerPaused = true; // Stop the timer when it reaches zero
    }
    lastTimerUpdateTime = currentMillis;
  }

  // Handle display blinking when paused
  if (timerPaused) {
    if (currentMillis - lastBlinkTime >= 300) {
      displayBlinkState = !displayBlinkState;
      lastBlinkTime = currentMillis;
    }
  } else {
    displayBlinkState = true;
  }

  // Adjust LED brightness when less than 20 minutes remain
  if (remainingTime <= 20UL * 60 * 1000) {
    float value = (20UL * 60 * 1000 - remainingTime) / (20UL * 60 * 1000.0);
    int brightness = convertToBrightness(value * 0.8);
    analogWrite(ledPin, brightness);
  } else {
    analogWrite(ledPin, 0);
  }

  // Turn off the display after 10 seconds of no interaction
  if (currentMillis - lastInteractionTime >= 10000) {
    displayOn = false;
  } else {
    displayOn = true;
  }

  // Display the remaining time
  if (displayOn) {
    if (displayBlinkState) {
      unsigned long totalMinutes = remainingTime / 60000UL;
      unsigned int hours = totalMinutes / 60;
      unsigned int minutes = totalMinutes % 60;
      int displayNumber = hours * 100 + minutes;
      sevseg.setNumber(displayNumber, 2);
    } else {
      sevseg.blank();
    }
  } else {
    sevseg.blank();
  }
}