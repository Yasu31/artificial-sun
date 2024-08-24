previous version of the project, which used an IR-controlled LED light strip for the light, with an IR LED connected to the Arduino (no custom board, no way to set the timer duration except for rewriting the sketch)

# artificial-sun DIY wake-up light alarm clock
Use an IR LED connected to an Arduino as a remote controller for LED light strip, to behave as a DIY wake-up light alarm clock. Useful when your country is way up north (or south) and the sun doesn't come up until late in winter.
Goes through a preset sequence of colors simulating sunset and sunrise.

I use an Intertronic RGB LED Strip, but I suspect the underlying protocol is the same for many other similar IR remote-controlled LEDs.

# how to use
connect LED strip to power, connect IR LED (with appropriate resistor) to pin 3.

# How each command ID was found; brute-force search!
https://www.instructables.com/BruteForce-Codefinding-for-Infrared-RGB-LEDstrip/
