# Lab 3 (Task 3)

## What is implemented
- USART2 TX/RX using interrupts:
  - TX path via TXE interrupt (`USART_CR1_TXEIE`).
  - RX path via RXNE interrupt.
- Function to start byte-array transmit packet.
- Button press (`PA0`) launches packet send.
- RX byte controls LED (`PC0`) by variant-specific OFF/ON values.

## Variant selection
- Open `Src/main.c` and set:
  - `#define LAB_VARIANT <your_variant>`
- Current setup in this project:
  - `#define LAB_VARIANT 5u`

## Proteus wiring additions
- Add Virtual Terminal.
- Connect:
  - `PA2` (USART2_TX) -> Virtual Terminal RX.
  - `PA3` (USART2_RX) <- Virtual Terminal TX.
- LED control:
  - `PC0` -> LED (with resistor).

## Runtime notes
- Baud rate in code: `9600`.
- Press button on `PA0` to send demo packet.
- Send OFF/ON bytes from terminal according to your variant mapping in code.

## Files for report
- Scheme screenshot from Proteus (with Virtual Terminal).
- Firmware listing: `Src/main.c`.
