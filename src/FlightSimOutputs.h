#ifndef _FLIGHTSIM_OUTPUTS_H
#define _FLIGHTSIM_OUTPUTS_H

#include <Arduino.h>


/*
 * Data outputs for Midwest737Simulations Multi Output card
 *
 * (c) Jorg Neves Bliesener
 */

const uint8_t TEENSY_DIN_PIN = 5;
const uint8_t TEENSY_CLK_PIN = 2;
const uint8_t TEENSY_ENA_PIN = 4;
const uint8_t TEENSY_STB_PIN = 3;
const uint8_t NO_ENA_PIN = 0xff;

const bool SR_74HCT4094 = false;
const bool SR_74HCT595 = true;

#define DEBUG_OFF (0)
#define DEBUG_OUTPUT (1)
#define DEBUG_VALUE (2)

const uint8_t NUMBER_OF_SHIFT_REGS = 6;
const uint8_t MAX_NUMBER_OF_CARDS = 8;
const uint8_t MAX_NUMBER_OF_SHIFT_REGS = MAX_NUMBER_OF_CARDS * NUMBER_OF_SHIFT_REGS;

const float DEFAULT_ON_THRESHOLD = 0.2;

class FlightSimOutputElement;

class MultiOutputBoard {
// The FLIGHTSIM_INTERFACE macro is defined in Teensy's runtime library, if it has been compiled with
// support for communications with X-Plane. Doesn't exist on the Arduino.
#ifdef FLIGHTSIM_INTERFACE
	friend FlightSimOutputElement;
#endif
public:
	MultiOutputBoard(const uint8_t numberOfShiftRegs = NUMBER_OF_SHIFT_REGS)
		: MultiOutputBoard(TEENSY_DIN_PIN, TEENSY_CLK_PIN, TEENSY_STB_PIN, TEENSY_ENA_PIN, false, numberOfShiftRegs) {}

	MultiOutputBoard(const uint8_t dinPin, const uint8_t clkPin, const uint8_t stbPin,
		const uint8_t enaPin=NO_ENA_PIN, bool enaIsActiveLow=false,
		const uint8_t numberOfShiftRegs = NUMBER_OF_SHIFT_REGS);

	void setDinPin(uint8_t dinPin) { this->dinPin = dinPin; }
	void setClkPin(uint8_t clkPin) { this->clkPin = clkPin; }
	void setStbPin(uint8_t stbPin) { this->stbPin = stbPin; }
	void setEnaPin(uint8_t enaPin, bool enaIsActiveLow=false) {
		this->enaPin = enaPin;
		this->enaIsActiveLow = enaIsActiveLow;
	}

	void printTime(Stream *s);
	bool checkInitialized(const char *message, bool mustBeInitialized);

	void begin();

	void setData(size_t pinNumber, bool value);
	bool getData(size_t pinNumber);

	void sendData();
	void sendDataIfChanged();
	void loop();

	static MultiOutputBoard *firstOutputBoard;

private:
	uint8_t numberOfShiftRegs;
	uint8_t dinPin;
	uint8_t clkPin;
	uint8_t stbPin;
	uint8_t enaPin;
	bool enaIsActiveLow;
	bool initialized;
	bool wasEnabled;
	bool valueChanged;
	uint8_t outputData[MAX_NUMBER_OF_SHIFT_REGS];

#ifdef FLIGHTSIM_INTERFACE
	FlightSimOutputElement *firstOutput;
	FlightSimOutputElement *lastOutput;
#endif
};



#ifdef FLIGHTSIM_INTERFACE
class FlightSimOutputElement {
	friend MultiOutputBoard;
public:
	FlightSimOutputElement(MultiOutputBoard *outBoard, size_t outputPin);
	virtual ~FlightSimOutputElement() {}
	void setDebug(uint8_t debugLevel) { this->debugLevel = debugLevel; }
	MultiOutputBoard *board;
	size_t outputPin;
protected:
	uint8_t debugLevel;
	bool isEnabled;
	virtual void begin(size_t maxPinNumber);

private:
	static FlightSimOutputElement *firstOrphan;
	static FlightSimOutputElement *lastOrphan;
	FlightSimOutputElement *next;

	virtual void forceUpdate() {}
};

class FlightSimDigitalOutput : public FlightSimOutputElement {

public:
	FlightSimDigitalOutput(size_t outputPin, float onThreshold=DEFAULT_ON_THRESHOLD, bool isInverted=false);
	FlightSimDigitalOutput(MultiOutputBoard *outputBoard, size_t outputPin, float onThreshold=DEFAULT_ON_THRESHOLD, bool isInverted=false);
	virtual ~FlightSimDigitalOutput() {}

	void setDataref(const _XpRefStr_ *positionDataref);
	void valueChanged(float newValue);
	FlightSimDigitalOutput & operator =(const _XpRefStr_ *s) { setDataref(s); return *this; }
	void setThreshold(float threshold) { this->onThreshold = threshold; }
private:
	const _XpRefStr_ *datarefName;
	FlightSimFloat dataref;
	float onThreshold;
	bool isInverted;

	virtual void begin(size_t maxPinNumber);
	virtual void forceUpdate();
};
#endif


#endif // _FLIGHTSIM_OUTPUTS_H
