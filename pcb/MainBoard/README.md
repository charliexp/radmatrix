# Main Board

50x50mm microcontroller module, driving LedArrayMux modules. Contains an RP2040 microcontroller, a CAN transceiver, PWM audio output, current sensing, and level shifters to work around LedArrayMux bugs.

## Revisions

### Rev A

Errata:

1. Level shifters are too slow to be useful
  - Workaround: decrease pullup values to obnoxiously low ones (100-180Ω); do not use level shifters; increase PIO delays until functional
2. CAN receive signal is not level shifted, which exposes RP2040 to a 5V signal
  - Workaround: Do nothing; replace with a 3v3 CAN transceiver
3. The 431 shunt voltage reference does not work correctly
  - Workaround: use main 3v3 regulator for vref
4. PWM audio signal filter is suspected to be responsible for poor quality audio
