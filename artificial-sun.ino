#include "SevSeg.h"
SevSeg sevseg; // Instantiate a seven-segment object

// Timer variables
unsigned long setDuration = 7UL * 60 * 60 * 1000; // User-set duration
unsigned long remainingTime = setDuration;        // Remaining time in milliseconds
unsigned long lastTimerUpdateTime = 0;
unsigned long currentMillis = 0;
bool timerPaused = true;

// Display variables
unsigned long lastInteractionTime = 0;
bool displayOn = true;
unsigned long lastBlinkTime = 0;
bool displayBlinkState = true;

// Center button long press duration in milliseconds
const unsigned long LONG_PRESS_DURATION = 1000;

// LED brightness variables
float currentBrightness = 0.0;
const float smoothingFactor = 0.005;

// LED auto-off variables
unsigned long ledTurnOffTime = 0;
const unsigned long LED_AUTO_OFF_DURATION = 60UL * 60 * 1000;

// Constants for acceleration effect
const unsigned long BASE_INTERVAL = 300; // Initial interval between increments (at start)
const unsigned long MIN_INTERVAL = 10;   // Minimum interval between increments (after pressing for long time)
const unsigned long MAX_PRESS_DURATION = 6000;

// pin assignments
const int buttonPin_l = 19;
const int buttonPin_c = 20;
const int buttonPin_r = 21;
const int ledPin = 6; // High-power LED pin


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

  bool checkButton() {
    // check the button state, with debouncing
    // returns true at the moment the button is pressed down
    const unsigned long debounceDelay = 50;
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

  // Serial print for debugging
  // don't use serial print on final uploaded code, since it delays the loop
  // Serial.begin(115200);
}

int convertToPWM(float brightnessValue) {
  // convert brightness value 0~1 to PWM value.
  // use exponential function so human can perceive the brightness linearly
  brightnessValue = constrain(brightnessValue, 0, 1);
  if (brightnessValue < 0.001)
    return 0;
  float e = 2.71828;
  // make it go between 0 ~ 255
  int pwmValue = pow(e, 5 * brightnessValue) / pow(e, 5) * 256 - 1;
  return pwmValue;
}

void handleTimeAdjustment(Button &button, bool isIncrement) {
  // isIncrement: true for the increment button, false for the decrement button
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
    // shorten the interval time between each step, the longer the button is pressed
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
  // don't blink if the time is being adjusted (i.e. left or right button is pressed)
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
      // e.g. 8 hours 10 mins -> show as "08.10" on the display
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
  // pulse it so that the brightness changes as a sine wave (for visual effect + not overheat the LEDs)
  float pulseFrequency = 0.1;  // Pulse frequency in Hz
  float sineFactor = (sin(2 * PI * pulseFrequency * (currentMillis / 1000.0)) + 1) / 2;  // changes between 0~1

  if (remainingTime <= rampup_dur) {
    targetMaxBrightness = (rampup_dur - remainingTime) / (float)rampup_dur;
    targetMaxBrightness = constrain(targetMaxBrightness, 0.06, 1);
  }
  else if ((setDuration - remainingTime) <= rampdown_dur && !timerPaused) {
    targetMaxBrightness = 1 - (setDuration - remainingTime) / (float)rampdown_dur;  // 1 -> 0
    targetMaxBrightness *= 0.2; // 0.2 -> 0  "sunset" effect should be darker than sunrise
    targetMaxBrightness += 0.06;  // 0.206 -> 0.006 add a small offset so the LED doesn't turn off completely
  } else {
    targetMaxBrightness = 0;
  }
  // smoothly pulse between 50% and 100% of the targetMaxBrightness
  targetBrightness = targetMaxBrightness * (0.5 + 0.5 * sineFactor);

  if (displayOn)
    targetBrightness = constrain(targetBrightness, 0, 0.03); // limit brightness

  // Turn off LED after auto-off duration
  if (ledTurnOffTime != 0 && currentMillis >= ledTurnOffTime) {
    targetBrightness = 0;
    ledTurnOffTime = 0;
  }

  // Exponential smoothing
  currentBrightness = smoothingFactor * targetBrightness +
                      (1.0 - smoothingFactor) * currentBrightness;

  // Serial.print("0., 1., ");  // to define the y axis range
  // Serial.print(targetMaxBrightness);
  // Serial.print(", ");
  // Serial.print(targetBrightness);
  // Serial.print(", ");
  // Serial.println(currentBrightness);

  int pwmValue =
      convertToPWM(currentBrightness);
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
