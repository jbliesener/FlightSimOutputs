#include <FlightSimOutputs.h>

/*
 * Data outputs for Midwest737Simulations Multi Output card
 *
 * (c) Jorg Neves Bliesener
 */

MultiOutputBoard *MultiOutputBoard::firstOutputBoard = nullptr;

#ifdef FLIGHTSIM_INTERFACE
FlightSimOutputElement *FlightSimOutputElement::firstOrphan = nullptr;
FlightSimOutputElement *FlightSimOutputElement::lastOrphan = nullptr;

void dataChanged(float newValue, void* context) {
	FlightSimDigitalOutput *output = (FlightSimDigitalOutput*) context;
	if (output) {
		output->valueChanged(newValue);
	}
}
#endif

MultiOutputBoard::MultiOutputBoard(const uint8_t dinPin, const uint8_t clkPin, const uint8_t stbPin,
	const uint8_t enaPin, bool enaIsActiveLow, const uint8_t numberOfShiftRegs) {
	this->dinPin = dinPin;
	this->clkPin = clkPin;
	this->stbPin = stbPin;
	this->enaPin = enaPin;
	this->enaIsActiveLow = enaIsActiveLow;
	this->numberOfShiftRegs = numberOfShiftRegs;
	this->initialized = false;
	this->wasEnabled = false;
	this->valueChanged = false;

	this->initialized = false;
	if (firstOutputBoard == nullptr) {
		firstOutputBoard = this;
	}

#ifdef FLIGHTSIM_INTERFACE
	this->firstOutput = nullptr;
	this->lastOutput = nullptr;
#endif
}

void MultiOutputBoard::printTime(Stream *s)
{
	char buf[13];

	sprintf(buf, "%10lu: ", millis());
	s->print(buf);
}

bool MultiOutputBoard::checkInitialized(const char *message, bool mustBeInitialized)
{
	if (initialized != mustBeInitialized) {
		printTime(&Serial);
		Serial.print("FlightSimOutputs ERROR: ");
		Serial.print(message);
		Serial.print(" must be called ");
		Serial.print(mustBeInitialized ? "after" : "before");
		Serial.print(" begin() ");
		if (mustBeInitialized)
			Serial.print("has been called with a correct setup.");
		Serial.println();
		return false;
	}
	return true;
}


void MultiOutputBoard::begin() {
	// check params
	if (numberOfShiftRegs > MAX_NUMBER_OF_SHIFT_REGS) {
		printTime(&Serial);
		Serial.print("MultiOutputBoard ERROR: Sorry, ");
		Serial.print(MAX_NUMBER_OF_SHIFT_REGS);
		Serial.println(" shift registers max");
		return;
	}

	// clear memory
	memset(outputData, 0, MAX_NUMBER_OF_SHIFT_REGS);

	// init pins
	pinMode(clkPin, OUTPUT);
	pinMode(dinPin, OUTPUT);
	pinMode(stbPin, OUTPUT);
	digitalWrite(clkPin, LOW);
	digitalWrite(dinPin, LOW);
	digitalWrite(stbPin, LOW);
	if (enaPin != NO_ENA_PIN) {
		pinMode(enaPin, OUTPUT);
		digitalWrite(enaPin, enaIsActiveLow ? LOW : HIGH);
	}

	// clear outputs at startup
	valueChanged=true;
	initialized=true;
	sendDataIfChanged();
	initialized=false;

#ifdef FLIGHTSIM_INTERFACE
	if (FlightSimOutputElement::firstOrphan != nullptr) {
		printTime(&Serial);
		Serial.println("FlightSimOutput ERROR: MultiOutputBoard MUST be declared before FlightSimOutputElement");
		return;
	}

	if (firstOutput == nullptr) {
		printTime(&Serial);
		Serial.println("MultiOutputBoard WARNING: No FlightSimOutputElement object defined");
	}

	size_t maxPinNumber = NUMBER_OF_SHIFT_REGS * 8;
	for (FlightSimOutputElement *o=firstOutput; o; o=o->next) {
		o->begin(maxPinNumber);
	}
#endif

	initialized = true;
}

void MultiOutputBoard::setData(size_t pinNumber, bool value) {
	if (!checkInitialized("MultiOutputBoard::setData", true))
		return;

	pinNumber -= 1;
	size_t srIndex = pinNumber >> 3;
	if (srIndex > numberOfShiftRegs) {
		printTime(&Serial);
		Serial.print("MultiOutputBoard ERROR: Trying to set output number ");
		Serial.print(pinNumber);
		Serial.print(", but this board only has ");
		Serial.print(numberOfShiftRegs * 8);
		Serial.println(" outputs.");
		return;
	}

	uint8_t bitMask = _BV(pinNumber & 7);
	if (value) {
		if (!(outputData[srIndex] & bitMask)) {
			valueChanged = true;
			outputData[srIndex] |= bitMask;
		}
	} else {
		if (outputData[srIndex] & bitMask) {
			valueChanged = true;
			outputData[srIndex] &= ~bitMask;
		}
	}
}

bool MultiOutputBoard::getData(size_t pinNumber) {
	if (!checkInitialized("MultiOutputBoard::getData()", true))
		return false;

	pinNumber -=1;
	size_t srIndex = pinNumber >> 3;
	if (srIndex > numberOfShiftRegs) {
		printTime(&Serial);
		Serial.print("MultiOutputBoard ERROR: Trying to read output number ");
		Serial.print(pinNumber);
		Serial.print(", but this board only has ");
		Serial.print(numberOfShiftRegs * 8);
		Serial.println(" outputs.");
		return false;
	}

	uint8_t bitMask = _BV(pinNumber & 7);
	return outputData[srIndex] & bitMask;
}

void MultiOutputBoard::sendData() {
	checkInitialized("MultiOutputBoard::sendData()", true);

	valueChanged = true;
	sendDataIfChanged();
}

void MultiOutputBoard::sendDataIfChanged() {
	checkInitialized("MultiOutputBoard::sendDataIfChanged()", true);

	if (!valueChanged)
		return;

	digitalWrite(stbPin, LOW);  // clear STB before sending data
	for (int8_t i=numberOfShiftRegs-1; i>=0; i--) { // start with last shift register
		// send data, MSB first
		shiftOut(dinPin, clkPin, MSBFIRST, outputData[i]);
	}
	digitalWrite(stbPin, HIGH); // latch data into output registers

	valueChanged = false;
}

void MultiOutputBoard::loop() {
	if (!checkInitialized("MultiOutputBoard::loop()", true))
		return;

#ifdef FLIGHTSIM_INTERFACE
	FlightSim.update();
	bool enabled = FlightSim.isEnabled();
	if (enabled != wasEnabled) {
		if (enabled) {
			printTime(&Serial);
			Serial.println("MultiOutputBoard: Connection with X-Plane established");
			for (FlightSimOutputElement *o=firstOutput; o; o=o->next) {
				if (o->isEnabled) {
					o->forceUpdate();
				}
			}
		}
		wasEnabled = enabled;
	}
#endif

	// send updated data
	sendDataIfChanged();
}


///////////

#ifdef FLIGHTSIM_INTERFACE
FlightSimOutputElement::FlightSimOutputElement(MultiOutputBoard *outputBoard, size_t outputPin) {
	this->outputPin = outputPin;
	this->board = outputBoard;
	this->next = nullptr;
	this->isEnabled = false;
	this->debugLevel = DEBUG_OFF;

	if (outputBoard != nullptr) {
		if (outputBoard->firstOutput == nullptr) {
			outputBoard->firstOutput = this;
		}

		if (outputBoard->lastOutput != nullptr) {
			outputBoard->lastOutput->next = this;
		}
		outputBoard->lastOutput = this;
	} else {
		if (firstOrphan == nullptr) {
			firstOrphan = this;
		}

		if (lastOrphan != nullptr) {
			lastOrphan->next = this;
		}
		lastOrphan = this;
	}
}

void FlightSimOutputElement::begin(size_t maxPinNumber) {
	if (outputPin > maxPinNumber) {
		board->printTime(&Serial);
		Serial.print("FlightSimOutputElement ERROR: FlightSimElement with pin number ");
		Serial.print(outputPin);
		Serial.print(" declared, but total number of output pins on this board is ");
		Serial.println(maxPinNumber);
		isEnabled = false;
	} else {
		board->printTime(&Serial);
		Serial.print("FlightSimOutputElement::begin(), pin ");
		Serial.println(outputPin);
		isEnabled = true;
	}
}

////////

FlightSimDigitalOutput::FlightSimDigitalOutput(size_t outputPin, float onThreshold, bool isInverted)
	: FlightSimDigitalOutput(MultiOutputBoard::firstOutputBoard, outputPin, onThreshold, isInverted) {}

FlightSimDigitalOutput::FlightSimDigitalOutput(MultiOutputBoard *outputBoard, size_t outputPin, float onThreshold, bool isInverted)
	: FlightSimOutputElement(outputBoard, outputPin) {
	this->dataref = nullptr;
	this->datarefName = nullptr;
	this->onThreshold = onThreshold;
	this->isInverted = isInverted;
}

void FlightSimDigitalOutput::begin(size_t maxPinNumber) {
	FlightSimOutputElement::begin(maxPinNumber);
	if (isEnabled) {
		board->printTime(&Serial);
		Serial.print("FlightSimDigitalOutput::begin(), dataref ");
		Serial.println((const char*) datarefName);
		if (datarefName == nullptr) {
			Serial.print("FlightSimDigitalOutput ERROR: FlightSimDigitalOutput for pin number ");
			Serial.print(outputPin);
			Serial.println(" has no dataref associated. Please assign one!");
			isEnabled = false;
		} else {
			dataref.onChange(dataChanged,(void*) this);
		}
	}
}

void FlightSimDigitalOutput::setDataref(const _XpRefStr_ *dataref) {
	this->dataref.assign(dataref);
	this->datarefName = dataref;
}

void FlightSimDigitalOutput::forceUpdate() {
	if (!isEnabled)
		return;

	valueChanged(dataref.read());
}

void FlightSimDigitalOutput::valueChanged(float newValue) {
	if (!isEnabled)
		return;

	bool newState = isInverted ? newValue <= onThreshold : newValue >= onThreshold;
	bool currentState = board->getData(outputPin);

	if (newState != currentState) {
		if (debugLevel>=DEBUG_OUTPUT) {
			board->printTime(&Serial);
			Serial.print("Dataref ");
			Serial.print((const char*) datarefName);
			Serial.print(" changed to ");
			Serial.print(newValue);
			Serial.print(", board output ");
			Serial.print(outputPin);
			Serial.print(" will be turned ");
			Serial.println(newState ? "ON" : "OFF");
		}
		board->setData(outputPin,newState);
	} else {
		if (debugLevel >= DEBUG_VALUE) {
			board->printTime(&Serial);
			Serial.print("Dataref ");
			Serial.print((const char*) datarefName);
			Serial.print(" changed to ");
			Serial.print(newValue);
			Serial.print(", no change on output pin ");
			Serial.println(outputPin);
		}
	}

}
#endif
