// This is a simple board test that just runs a test sequence on the Multi Output board.
// The first test will turn on each group of eight bits for 220 milliseconds.
// Then, a single bit is endlessly cycled through all outputs on the board in intervals
// of 100 milliseconds.
//
// This sketch doesn't require a Teensy on board, nor does it use any Flightsim related
// functions. It works with any Arduino board.

#include <Arduino.h>
#include <FlightSimOutputs.h>

// Teensy on board: standard pin assignment. Comment out if you want to use with Arduino
MultiOutputBoard board;

// For Arduino, use this line and specify pin numbers. For use with 74HCT595 shift registers,
// set enaIsActiveLow to true. If ENA pin is tied to active in hardware, use NO_ENA_PIN
// MultiOutputBoard board(/*DIN_PIN*/5, /*CLK_PIN*/2, /*STB_PIN*/3,
//	/*ENA_PIN: unused*/NO_ENA_PIN, /*ENA is activeLow*/false);

const size_t NUM_SHIFT_REGS = 6;

size_t outputNumber;

void setup() {
	// call MultiOutputBoard.begin once to set everything up
	board.begin();

	// short blink on output groups
	for (size_t i = 1; i <= NUM_SHIFT_REGS; i++) {

		// turn group of 8 outputs on
		for (size_t j = i * 8 - 7; j <= i * 8; j++) {
			board.setData(j, true);
		}

		// MultiOutputBoard.sendData() always sends out the data, regardless if there
		// was a change or not.
		board.sendData();

		// turn outputs off again
		for (size_t j = i * 8 - 7; j <= i * 8; j++) {
			board.setData(j, false);
		}

		delay(200);
	}

	// turn remaining outputs off
	board.sendData();

	outputNumber = 1;
}

void loop() {
	// cycle through the output bits, one by one
	board.setData(outputNumber, true);

	// MultiOutputBoard.sendDataIfChanged() only actually sends the
	// data if there was any change from the last time
	board.sendDataIfChanged();

	delay(100);
	board.setData(outputNumber, false);

	outputNumber++;
	if (outputNumber > NUM_SHIFT_REGS * 8) {
		outputNumber = 1;
	}
}
