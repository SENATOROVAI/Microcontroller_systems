# Lab 1 (Task 1)

## What is implemented
- Microcontroller firmware for:
  - `UP` button: increment 8-bit counter.
  - `DOWN` button: decrement 8-bit counter.
  - 8 LEDs: show counter in binary (`PC0..PC7`).
- Initial state: all LEDs are OFF.

## Proteus wiring
- MCU: STM32F401RE (or compatible STM32F4 in your project).
- Buttons:
  - `UP` -> `PA0` (active LOW, internal pull-up is enabled).
  - `DOWN` -> `PA1` (active LOW, internal pull-up is enabled).
- LEDs:
  - `PC0..PC7` to 8 LEDs (with proper resistors).

## Files for report
- Scheme screenshot from Proteus.
- Firmware listing: `Src/main.c`.
