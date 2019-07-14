// This example shows how to use the FlightSimSwitch library with
// the stock X-Plane Cessna 172 to show the transponder activity
// lights on the output card.
//
// This sketch requires a Teensy on the card. It will not work with
// Arduino boards.
//
// Make sure to set the hardware configuration in the Arduino IDE
// to "Board: Teensy 3.1/3.1" or "Board: Teensy LC" and USB Type to
// "Flight Sim Controls", otherwise the sketch won't compile.
//
// The serial console will show debug information selected further
// down.

#include <FlightSimOutputs.h>

// Declaring the MultiOutputBoard object. This object must always be
// declared BEFORE any specific FlightSimOutput object
MultiOutputBoard board;

// Now we configure our outputs
// The first output object is the transponder activity light on output O1.
// It will turn on whenever the dataref value (see below) rises above 0.2
// (the default threshold)
FlightSimDigitalOutput transponderActive(1);

// We will put the same dataref on output O2, but with a higher threshold
// value of 0.5. We will see that it turns on a bit later and turns off a bit
// earlier.
FlightSimDigitalOutput transponderActiveHighThreshold(2, 0.5);

// Finally, we will put define a third output on O3 that shows the transponder
// activity light inverted: Whenever the dataref value falls BELOW 0.3, we will
// turn the output on
FlightSimDigitalOutput transponderActiveInverted(3, 0.3, true);


void setup() {
  // Serial output stuff, see Wiki (https://www.github.com/jbliesener/FlightSimSwitches/wiki/Serial)
  delay(2000);         // Teensy startup delay
  Serial.begin(9600);
  FlightSim.update();  // Call FlightSim.update to make serial output work. Must not take longer
                       // than 2 seconds for the next call to FlightSim.update()
  Serial.println(__FILE__ ", compiled on " __DATE__ " at "  __TIME__);

  // If you use 74HCT595 shift registers, you must call this function once with "true"
  // as the second parameter, as the 74HCT595 uses an active LOW signal on the ENA_PIN.
  // 74HCT4094 uses active HIGH, which is the default.
  // board.setEnaPin(TEENSY_ENA_PIN, true);

  // Set the X-Plane datarefs. The first three outputs all read from the same dataref
  transponderActive=             XPlaneRef("sim/cockpit2/radios/indicators/transponder_brightness");
  transponderActiveHighThreshold=XPlaneRef("sim/cockpit2/radios/indicators/transponder_brightness");
  transponderActiveInverted=     XPlaneRef("sim/cockpit2/radios/indicators/transponder_brightness");

  // provide a little feedback on the serial console for two of the datarefs
  transponderActiveInverted.setDebug(DEBUG_VALUE); // report every value change
  transponderActiveHighThreshold.setDebug(DEBUG_OUTPUT); // report only changes in board output

  // Do not forget to call MultiOutputBoard.begin()!!
  board.begin();
}

void loop() {
  // A single call to MultiOutputBoard.loop() does it all here
  board.loop();
}
