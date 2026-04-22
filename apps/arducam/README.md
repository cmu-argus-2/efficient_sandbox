# arducam

Fresh E1x app scaffold for bringing up the ArduCAM OV2640 camera in a clean,
example-style structure.

Planned direction:

- `main.c`: E1x-specific board wiring, pinmux, and SDK peripheral setup
- `arducam_spi.*`: portable ArduChip SPI-side FIFO/capture control
- `ov2640_sccb.*`: portable OV2640 SCCB/I2C register access and ID checks
