# radmatrix

Smol modular LED matrix, consisting of 3 types of 50x50mm modules:

- LED modules with a 20x20 matrix of 0603 red LEDs
- Driver modules at the end of rows and columns (bottom and right)
- Microcontroller (RP2040) board in the bottom-right corner

This was built as my first "serious" electronics project and its main purpose was to learn electronics a bit better. Some RP2040 embedded knowledge and a blinky lights object are nice side effects.

## Results

TODO: Photos, video, PCB renders

## Design details

The LED matrix and driver module PCB design were optimized for JLCPCB Assembly - i.e. only using parts available in its (LCSC) catalog and strongly preferring Basic parts to avoid paying feeder fees. The LEDs chosen were the cheapest available in JLC's catalog (they work fine btw).

The microcontroller board was optimized for JLC PCBA to a lesser extent -- I used multiple parts that would incur a feeder fee if not for the fact that I hand-soldered them.

The display is capable of:

- 8 bit red color
- 60fps (TODO, 30fps demonstrated)

Additionally, the MCU board can:

- Read multiple "videos" (specially encoded for this firmware) from an SD card
- Pipe in 8-bit 44kHz audio
- Transmit and receive CAN messages

## v2

Now that I've leraned from my mistakes, here's what I would do if I were to build a v2 of this display (I probably won't):

- Use square, preferably RGB LED "pixels"
- Fit them in a grid of **standard** spacing (i.e. 1, 1.27, 2, or 2.54mm)
- Put pixels edge to edge
  - Current design simplfiied connections and JLC assembly, but it's ugly
- Use a black PCB for the LED matrix for increased contrast
- Move driver modules from column/row ends to the backs of LED modules
  - The current design is kinda neat, but it doesn't scale, as with each additional row of modules, the perceptual brightness of LEDs goes down
- A 2mm-spacing 32x32 standard module seems reasonable
- Connection between LED module and driver module would be done by placing 1x16 SMD pin headers alongside corners (split in half, say, bottom-left and top-right for rows, and likewise for columns to allow edge-to-edge clearance)
  - Possibly some additional pin headers for added stability, maybe some solderable nuts - some mechanical connection between modules would be nice
- Driver modules
  - Would consider constant-current drivers, although resistor-based approach seems to work fine and appeared cheaper
  - If resistor-based approach is still used, I would consider a buck converter for creating a variable LED power supply
  - Would consider not skipping any of the shift register stages as that makes programming more annoying, and in this approach, it shouldn't make layout too difficult
  - Fix v1 bugs of course
  - Would consider faster shift registers. Perhaps 74HCT (5V supply to '595 gives best performance, and HCT can handle 3v3 input), or LV*
  - Would consider a decoder instead of shift registers for selecting rows. The current method is risky in that if programming is not done correctly, multiple LED rows could be lighting up at the same time (with one resistor for multiple LEDs), overstressing them. It's likely that current approach is cheaper and requires fewer signal lines though
- Driving the display
  - Each LED/driver module would have a separate connection to the MCU board - sharing row selection, OE, SRCLK, RCLK, lines, but each having its own SER line
  - With PIO, this would have minimal impact on driving performance, and RP2040 IO counts would likely allow for 16-20 module to be connected to a single MCU board
  - This could in principle be extended further with multiple MCU boards talking to a master
  - The display would become /32, so would be brighter and that would not depend on its height
  - Each driver module should have a buffer before/after it to improve the signal

Note that while most people do projects like this with an FPGA, in my opinion the RP2040, thanks to PIO, is more than capable enough for this job at a better price, and can do other stuff in addition to just driving the display. The main downside vs an FPGA is limited IO (30 pins), which limits how big of a display can be driven. With the design above, likely ~20kpx (though this could be optimized further)

Noting the above so I don't forget or in case someone else wants to take this project further. To me, this project has fulfilled its educational goal, and making large LED displays this way is impractical given that you can get HUB75 modules for less than just the LED cost off LCSC.
