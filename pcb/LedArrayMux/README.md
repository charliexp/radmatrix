# LED Multiplexer

50x50mm module for driving rows and columns of LED Matrix modules

## Revisions

### Rev A

Manufactured 2024-04-06, delivered 2024-04-13.

Errata:

1. PMOS gates are pulled up to VLED (5V), while the shift register is powered from 3V3 and signals above VCC are not allowed. Ouch!
   - NMOS gates (i.e. row drivers) are not affected
   - This is not easily fixable on the board itself
   - It doesn't appear that 74HC595 dies immediately because of this, and the 10k resistor would limit worst-case scenario current to 0.5mA, but probably bad idea to ignore this altogether
   - It's possible to set VCC to 5V, and add level shifting on signal lines
   - Alternatively, the shift register could be replaced with 74HCT595 so that 3V3 signals register as logic high at 5V VCC
