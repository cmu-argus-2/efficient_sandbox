#include <stdio.h>
#include <eff.h>
#include <eff/drivers/spi.h>

int main(void)
{
    eff_pinmux_set(PINMUX_0, PINMUX_SPI);

    eff_spi_cfg_t cfg = EFF_SPI_DEFAULTS;
    cfg.xfer_mode = SPI_XFER_WRITE_ONLY;
    cfg.bus_size  = SPI_BUS_SINGLE;

    if (eff_spi_init(SPI_0, &cfg) != 0) {
        printf("spi_init failed\n");
        return -1;
    }

    uint8_t tx[16] = {
        0xAA, 0x55, 0xAA, 0x55,
        0xF0, 0x0F, 0xCC, 0x33,
        0xAA, 0x55, 0xAA, 0x55,
        0xF0, 0x0F, 0xCC, 0x33
    };

    while (1) {
        eff_spi_xfer(SPI_0, 0, 0, tx, sizeof(tx), 0, 0);
        sleep(1);
    }
}