#include "SmartLED.h"

SmartLED::SmartLED(uint8_t pin, bool activeHigh)
  : _pin(pin), _activeHigh(activeHigh), _state(false) {}

void SmartLED::begin() {
  pinMode(_pin, OUTPUT);
  applyState();
}

void SmartLED::turnOn() {
  _state = true;
  applyState();
}

void SmartLED::turnOff() {
  _state = false;
  applyState();
}

void SmartLED::toggle() {
  _state = !_state;
  applyState();
}

bool SmartLED::isOn() const {
  return _state;
}

uint8_t SmartLED::getPin() const {
  return _pin;
}

void SmartLED::applyState() {
  uint8_t level = _state ? (_activeHigh ? HIGH : LOW) : (_activeHigh ? LOW : HIGH);
  digitalWrite(_pin, level);
}
