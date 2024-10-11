#include "SevSeg.h"
SevSeg sevseg; // Instantiate a seven-segment object

// Timer variables
unsigned long lastInteractionTime = 0;
bool displayOn = true;
unsigned long setDuration = 7UL * 60 * 60 * 1000; // User-set duration
unsigned long remainingTime = setDuration;        // Remaining time in milliseconds
bool timerPaused = true;
unsigned long lastBlinkTime = 0;
bool displayBlinkState = true;
unsigned long lastTimerUpdateTime = 0;

// Center button long press duration
const unsigned long LONG_PRESS_DURATION = 1000; // 1 second for long press

// LED brightness variables
float currentBrightness = 0.0;
const float smoothingFactor = 0.002;

// LED auto-off variables
unsigned long ledTurnOffTime = 0;
const unsigned long LED_AUTO_OFF_DURATION = 60UL * 60 * 1000;

// Constants for acceleration effect
const unsigned long BASE_INTERVAL = 300; // Initial interval between increments
const unsigned long MIN_INTERVAL = 10;   // Minimum interval between increments
const unsigned long MAX_PRESS_DURATION = 6000;

// Button pins
const int buttonPin_l = 19;
const int buttonPin_c = 20;
const int buttonPin_r = 21;
const int ledPin = 6; // High-power LED pin

// Current time in milliseconds
unsigned long currentMillis = 0;

// Button class to handle debouncing and state
class Button {
public:
  int pin;
  bool state;
  bool lastState;
  unsigned long lastDebounceTime;
  bool lastButtonPushed;
  unsigned long pressStartTime;
  bool ignoreCurrentPress;
  unsigned long lastIncrementTime;

  Button(int pin) : pin(pin), state(HIGH), lastState(HIGH), lastDebounceTime(0),
                    lastButtonPushed(false), pressStartTime(0),
                    ignoreCurrentPress(false), lastIncrementTime(0) {}

  bool checkButton(unsigned long debounceDelay = 50) {
    int reading = digitalRead(pin);

    if (reading != lastState) {
      lastDebounceTime = currentMillis;
    }

    if ((currentMillis - lastDebounceTime) > debounceDelay) {
      if (reading != state) {
        state = reading;
        if (state == LOW) {
          return true; // Button down event
        }
      }
    }

    lastState = reading;
    return false;
  }

  bool isPressed() {
    return state == LOW;
  }

  bool isReleased() {
    return (!isPressed() && lastButtonPushed);
  }

  void updateLastButtonPushed() {
    lastButtonPushed = isPressed();
  }
};

// Instantiate button objects
Button leftButton(buttonPin_l);
Button centerButton(buttonPin_c);
Button rightButton(buttonPin_r);

void setup() {
  byte numDigits = 4;
  byte digitPins[] = {5, 9, 10, 15};
  byte segmentPins[] = {7, 14, 17, 3, 2, 8, 16, 4};
  bool resistorsOnSegments = false; // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_ANODE;
  bool updateWithDelays = false;
  bool leadingZeros = false;
  bool disableDecPoint = false;
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins,
               resistorsOnSegments, updateWithDelays, leadingZeros,
               disableDecPoint);

  pinMode(ledPin, OUTPUT);
  pinMode(leftButton.pin, INPUT_PULLUP);
  pinMode(centerButton.pin, INPUT_PULLUP);
  pinMode(rightButton.pin, INPUT_PULLUP);

  lastInteractionTime = millis();
}

int convertToPWM(float brightnessValue) {
  brightnessValue = constrain(brightnessValue, 0, 1);
  if (brightnessValue < 0.001)
    return 0;
  float e = 2.71828; // Euler's number
  int pwmValue = pow(e, 5 * brightnessValue) / pow(e, 5) * 255;
  return pwmValue;
}

void handleTimeAdjustment(Button &button, bool isIncrement) {
  if (button.checkButton()) {
    lastInteractionTime = currentMillis;
    if (!displayOn) {
      displayOn = true;
      button.ignoreCurrentPress = true;
    } else {
      button.pressStartTime = currentMillis;
      button.ignoreCurrentPress = false;
    }
  }

  if (displayOn && !button.ignoreCurrentPress && button.isPressed() &&
      timerPaused) {
    unsigned long pressDuration = currentMillis - button.pressStartTime;
    unsigned long interval = BASE_INTERVAL -
                             (BASE_INTERVAL - MIN_INTERVAL) *
                                 constrain((float)pressDuration /
                                               MAX_PRESS_DURATION,
                                           0, 1);
    interval = constrain(interval, MIN_INTERVAL, BASE_INTERVAL);

    if ((currentMillis - button.lastIncrementTime) >= interval) {
      unsigned long adjustAmount = 1UL * 60 * 1000; // 1 minute

      if (isIncrement) {
        remainingTime += adjustAmount;
      } else {
        remainingTime = remainingTime >= adjustAmount
                            ? remainingTime - adjustAmount
                            : 0;
      }
      setDuration = remainingTime; // Update setDuration
      button.lastIncrementTime = currentMillis;
    }
    lastInteractionTime = currentMillis;
  }

  if (button.isReleased()) {
    button.pressStartTime = 0;
    button.ignoreCurrentPress = false;
  }

  button.updateLastButtonPushed();
}

void handleCenterButton(Button &button) {
  static bool centerButtonLongPressHandled = false;
  static unsigned long centerButtonPressTime = 0;

  if (button.checkButton()) {
    centerButtonPressTime = currentMillis;
    centerButtonLongPressHandled = false;
    lastInteractionTime = currentMillis;
    if (!displayOn) {
      displayOn = true;
      button.ignoreCurrentPress = true;
    } else {
      button.ignoreCurrentPress = false;
    }
  }

  if (displayOn && !button.ignoreCurrentPress && button.isPressed()) {
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

  if (button.isReleased()) {
    unsigned long pressDuration = currentMillis - centerButtonPressTime;
    if (displayOn && !centerButtonLongPressHandled &&
        pressDuration < LONG_PRESS_DURATION && !button.ignoreCurrentPress) {
      // Short press detected
      timerPaused = !timerPaused; // Toggle pause/restart
      lastInteractionTime = currentMillis;
    }
    centerButtonPressTime = 0;
    button.ignoreCurrentPress = false;
  }

  button.updateLastButtonPushed();
}

void updateTimer() {
  if (!timerPaused && (currentMillis - lastTimerUpdateTime >= 1000)) {
    remainingTime = remainingTime >= 1000 ? remainingTime - 1000 : 0;

    if (remainingTime == 0) {
      timerPaused = true; // Stop the timer when it reaches zero
      // Start LED auto-off timer
      ledTurnOffTime = currentMillis + LED_AUTO_OFF_DURATION;
    }
    lastTimerUpdateTime = currentMillis;
  }
}

void updateDisplay() {
  // Handle display blinking when paused
  if (timerPaused && !(leftButton.isPressed() || rightButton.isPressed())) {
    if (currentMillis - lastBlinkTime >= 300) {
      displayBlinkState = !displayBlinkState;
      lastBlinkTime = currentMillis;
    }
  } else {
    displayBlinkState = true;
  }

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

void updateBrightness() {
  // Adjust LED brightness
  float targetBrightness = 0;
  unsigned long rampdown_dur = 10UL * 60 * 1000; // gradually lower brightness ("sunset" effect)
  unsigned long rampup_dur = 20UL * 60 * 1000; // gradually increase brightness ("sunrise" effect)
  float targetMaxBrightness = 0;
  // pulse it so that the brightness changes between 50% and 100% of the targetMaxBrightness
  float pulseFrequency = 0.1;  // Pulse frequency in Hz
  float sineFactor = (sin(2 * PI * pulseFrequency * (currentMillis / 1000.0)) + 1) / 2;  // changes between 0~1

  if ((setDuration - remainingTime) <= rampdown_dur) {
    targetMaxBrightness = 1 - (setDuration - remainingTime) / (float)rampdown_dur;
    targetMaxBrightness *= 0.3; // "sunset" effect should be darker than sunrise
  } else if (remainingTime <= rampup_dur) {
    targetMaxBrightness = (rampup_dur - remainingTime) / (float)rampup_dur;
    targetMaxBrightness = constrain(targetMaxBrightness, 0, 1);
  } else {
    targetMaxBrightness = 0;
  }
  targetBrightness = targetMaxBrightness * (0.5 + 0.5 * sineFactor);

  if (displayOn)
    targetBrightness = constrain(targetBrightness, 0, 0.05); // Cap at 5%

  // Turn off LED after auto-off duration
  if (ledTurnOffTime != 0 && currentMillis >= ledTurnOffTime) {
    targetBrightness = 0;
    ledTurnOffTime = 0;
  }

  // Exponential smoothing
  currentBrightness = smoothingFactor * targetBrightness +
                      (1.0 - smoothingFactor) * currentBrightness;

  int pwmValue =
      convertToPWM(constrain(currentBrightness, 0, 1) * 0.8); // Max 80%
  analogWrite(ledPin, pwmValue);
}

void checkInactivity() {
  // Turn off the display after 10 seconds of no interaction
  displayOn = (currentMillis - lastInteractionTime < 10000);
}

void loop() {
  currentMillis = millis();
  sevseg.refreshDisplay();

  // Handle buttons
  handleTimeAdjustment(leftButton, false); // Decrement time
  handleTimeAdjustment(rightButton, true); // Increment time
  handleCenterButton(centerButton);

  // Update timer and display
  updateTimer();
  updateDisplay();
  updateBrightness();
  checkInactivity();
}
