#include "SevSeg.h"
SevSeg sevseg; // Instantiate a seven-segment object
// https://github.com/DeanIsMe/SevSeg

// Variables for the timer
unsigned long lastInteractionTime = 0;
bool displayOn = true;
unsigned long setDuration = 7UL * 60 * 60 * 1000; // User-set duration
unsigned long remainingTime = setDuration; // Remaining time in milliseconds
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
unsigned long leftButtonPressStartTime = 0;

// Variables for right button
unsigned long lastIncrementTime_r = 0;
unsigned long rightButtonPressStartTime = 0;

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

// Variables for LED brightness smoothing
float currentBrightness = 0.0;
const float smoothingFactor = 0.002;

// Variable for LED auto-off
unsigned long ledTurnOffTime = 0;
const unsigned long LED_AUTO_OFF_DURATION = 60UL * 60 * 1000;

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

int convertToPWM(float brightnessValue) {
  // convert value between 0~1 to brightness value between 0~255 using an exponential function so that it feels natural to a human
  brightnessValue = constrain(brightnessValue, 0, 1);
  if (brightnessValue== 0)
    return 0;
  float e = 2.71828; // Euler's number
  int pwmValue = pow(e, 5*brightnessValue)/pow(e,5) * 255;
  return pwmValue;
}

void loop() {
  unsigned long currentMillis = millis();
  sevseg.refreshDisplay();

  // read the button output
  // note: buttonDown is true if the button was pressed down in this step, buttonPushed is whether it is currently pressed
  bool buttonDown_l = checkButton(buttonPin_l, buttonState_l, lastButtonState_l, lastDebounceTime_l);
  bool buttonPushed_l = (buttonState_l == LOW);
  bool buttonReleased_l = (!buttonPushed_l && lastButtonPushed_l);

  bool buttonDown_c = checkButton(buttonPin_c, buttonState_c, lastButtonState_c, lastDebounceTime_c);
  bool buttonPushed_c = (buttonState_c == LOW);
  bool buttonReleased_c = (!buttonPushed_c && lastButtonPushed_c);

  bool buttonDown_r = checkButton(buttonPin_r, buttonState_r, lastButtonState_r, lastDebounceTime_r);
  bool buttonPushed_r = (buttonState_r == LOW);
  bool buttonReleased_r = (!buttonPushed_r && lastButtonPushed_r);

  // Handle left button (decrement time)
  if (buttonDown_l) {
    lastInteractionTime = currentMillis;
    if (!displayOn) {
      displayOn = true;
    } else {
      leftButtonPressStartTime = currentMillis;
    }
  }

  if (displayOn && buttonPushed_l && timerPaused) {
    unsigned long pressDuration = currentMillis - leftButtonPressStartTime;
    unsigned long interval = 300; // ms
    if ((currentMillis - lastIncrementTime_l) >= interval) {
      unsigned long decrementAmount;
      if (pressDuration < 1000) {
        decrementAmount = 1UL * 60 * 1000; // 1 minute
      } else if (pressDuration < 3000) {
        decrementAmount = 5UL * 60 * 1000; // 5 minutes
      } else {
        decrementAmount = 15UL * 60 * 1000; // 15 minutes
      }
      if (remainingTime >= decrementAmount) {
        remainingTime -= decrementAmount;
      } else {
        remainingTime = 0;
      }
      setDuration = remainingTime; // Update setDuration
      lastIncrementTime_l = currentMillis;
    }
    lastInteractionTime = currentMillis;
  }

  if (buttonReleased_l) {
    leftButtonPressStartTime = 0;
  }
  lastButtonPushed_l = buttonPushed_l;

  // Handle right button (increment time)
  if (buttonDown_r) {
    lastInteractionTime = currentMillis;
    if (!displayOn) {
      displayOn = true;
    } else {
      rightButtonPressStartTime = currentMillis;
    }
  }

  if (displayOn && buttonPushed_r && timerPaused) {
    unsigned long pressDuration = currentMillis - rightButtonPressStartTime;
    unsigned long interval = 300; // ms
    if ((currentMillis - lastIncrementTime_r) >= interval) {
      unsigned long incrementAmount;
      if (pressDuration < 1000) {
        incrementAmount = 1UL * 60 * 1000; // 1 minute
      } else if (pressDuration < 3000) {
        incrementAmount = 5UL * 60 * 1000; // 5 minutes
      } else {
        incrementAmount = 15UL * 60 * 1000; // 15 minutes
      }
      remainingTime += incrementAmount;
      setDuration = remainingTime; // Update setDuration
      lastIncrementTime_r = currentMillis;
    }
    lastInteractionTime = currentMillis;
  }

  if (buttonReleased_r) {
    rightButtonPressStartTime = 0;
  }
  lastButtonPushed_r = buttonPushed_r;

  // Handle center button (pause/restart and reset)
  if (buttonDown_c) {
    centerButtonPressTime = currentMillis;
    centerButtonLongPressHandled = false;
    lastInteractionTime = currentMillis;
    if (!displayOn) {
      displayOn = true;
    }
  }

  if (displayOn && buttonPushed_c) {
    if (!centerButtonLongPressHandled) {
      unsigned long pressDuration = currentMillis - centerButtonPressTime;
      if (pressDuration >= LONG_PRESS_DURATION) {
        // Long press detected
        centerButtonLongPressHandled = true;
        remainingTime = setDuration; // Reset to previously set duration
        timerPaused = true;
        lastInteractionTime = currentMillis;
        displayOn = true;
      }
    }
  }

  if (buttonReleased_c) {
    unsigned long pressDuration = currentMillis - centerButtonPressTime;
    if (displayOn && !centerButtonLongPressHandled && pressDuration < LONG_PRESS_DURATION) {
      // Short press detected
      timerPaused = !timerPaused; // Toggle pause/restart
      lastInteractionTime = currentMillis;
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
      // Start LED auto-off timer
      ledTurnOffTime = currentMillis + LED_AUTO_OFF_DURATION;
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

  // Adjust LED brightness
  float targetBrightness = 0;
  unsigned long rampup_dur = 20UL * 60 * 1000; // how long to ramp up to full brightness
  if (remainingTime <= rampup_dur) {
    targetBrightness = (rampup_dur - remainingTime) / rampup_dur;
  } else {
    targetBrightness = 0;
  }

  if (timerPaused)
    targetBrightness = constrain(targetBrightness, 0, 0.2); // Cap brightness to 20%

  // Turn off LED after auto-off duration
  if (ledTurnOffTime != 0 && currentMillis >= ledTurnOffTime) {
    targetBrightness = 0;
    ledTurnOffTime = 0;
  }

  // Exponential smoothing
  currentBrightness = smoothingFactor * targetBrightness + (1.0 - smoothingFactor) * currentBrightness;

  int pwmValue = convertToPWM(constrain(currentBrightness, 0, 1) * 0.8); // apply 80% limit for safety
  analogWrite(ledPin, pwmValue);

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
