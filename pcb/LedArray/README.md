# LED Matrix module

50x50mm module with 20x20 LED matrix

## Errata

### Rev A

1. I'm an idiot and somehow managed to switch polarity of LEDs. Rows were supposed to be cathodes, and columns anodes, but it's the other way around. Fixable by a 90° rotation, albeit with less than ideal trace layout. Oh well.
2. The notes about the XL-1608SURC-06 LED are incorrect, and were in fact taken from XL-1608SURC-04's data sheet. (Correction: It seems that the ealier version of XL-1608SURC-06 datasheet was incorrect). The correct info is:
   - Forward voltage: 1.9-2.3V (at 20mA)
   - Max peak forward current: 80mA
   - Max continuous forward current: 20mA
