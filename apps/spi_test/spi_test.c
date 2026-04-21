#include <stdio.h>
#include <eff.h>
#include <eff/drivers/spi.h>

int main(void)
{
    printf("start\n");

    // Put Bank 0 into SPI mode
    eff_pinmux_set(PINMUX_0, PINMUX_SPI);
    printf("pinmux ok\n");

    eff_spi_cfg_t cfg = EFF_SPI_DEFAULTS;
    cfg.xfer_mode = SPI_XFER_WRITE_ONLY;
    cfg.bus_size  = SPI_BUS_SINGLE;

    int8_t rc = eff_spi_init(SPI_0, &cfg);
    if (rc != 0) {
        printf("spi_init failed: %d\n", rc);
        return -1;
    }
    printf("spi_init ok\n");

    uint8_t tx[4] = {0xAA, 0x55, 0x12, 0x34};

    while (1) {
        printf("xfer\n");
        eff_spi_xfer(SPI_0, 0, 0, tx, sizeof(tx), 0, 0);
        sleep(1);
    }

    return 0;
}