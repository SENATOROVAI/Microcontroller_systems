# Lab 2 (Task 2)

## What is implemented
- All Task 1 behavior preserved.
- PLL-based system clock setup from variant table (`84..44 MHz`):
  - set Flash latency,
  - set PLL source,
  - set PLL factors,
  - enable PLL,
  - switch main clock to PLL.
- SysTick configured to `10 kHz`.
- In `SysTick_Handler`, pin `PB0` toggles state for oscilloscope measurement.

## Variant selection
- Open `Src/main.c` and set:
  - `#define LAB_VARIANT <your_variant>`
- Current setup in this project:
  - `#define LAB_VARIANT 5u`

## Proteus wiring additions
- Add oscilloscope channel to `PB0`.
- Verify output frequency from `PB0`.

## Files for report
- Scheme screenshot from Proteus (with oscilloscope).
- Firmware listing: `Src/main.c`.
- Oscilloscope screenshot with measured frequency.
