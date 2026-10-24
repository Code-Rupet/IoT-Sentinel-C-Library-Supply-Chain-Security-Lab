#ifndef SMART_LED_H
#define SMART_LED_H

#include <Arduino.h>

/**
 * SmartLED Library
 * ================
 * A clean, reusable hardware abstraction for controlling indicator LEDs.
 * Supports configurable GPIO, active-high / active-low logic levels,
 * state persistence, and state query interfaces.
 */
class SmartLED {
public:
  /**
   * Constructs a SmartLED instance.
   *
   * @param pin         GPIO pin number connected to the LED
   * @param activeHigh  true if HIGH level illuminates LED, false if active-low
   */
  SmartLED(uint8_t pin, bool activeHigh = true);

  /**
   * Initializes the GPIO pin mode and applies default initial state (OFF).
   */
  void begin();

  /**
   * Powers the LED ON.
   */
  void turnOn();

  /**
   * Powers the LED OFF.
   */
  void turnOff();

  /**
   * Inverts current LED state.
   */
  void toggle();

  /**
   * Returns true if the LED is currently ON.
   */
  bool isOn() const;

  /**
   * Returns configured GPIO pin.
   */
  uint8_t getPin() const;

private:
  uint8_t _pin;
  bool _activeHigh;
  bool _state;

  void applyState();
};

#endif // SMART_LED_H
