#include <eff.h>
#include <eff/drivers/i2c.h>
#include <eff/drivers/spi.h>
#include <stdio.h>

#include "arducam_spi.h"
#include "ov2640_sccb.h"

/* Board-specific E1x wiring for this app. */
#define CAM_SPI_PINMUX PINMUX_2
#define CAM_SPI        SPI_2

#define CAM_I2C_PINMUX PINMUX_4
#define CAM_I2C        I2C_4_1
#define CAM_I2C_SPEED  I2C_SPEED_100K

#define OV2640_I2C_ADDR 0x30u

static int8_t e1x_spi_transact(void *ctx,
                               const uint8_t *tx,
                               uint32_t tx_len,
                               uint8_t *rx,
                               uint32_t rx_len)
{
    eff_spi_t *spi = (eff_spi_t *)ctx;
    return eff_spi_xfer(spi, 0u, 0u, (uint8_t *)tx, tx_len, rx, rx_len);
}

static int8_t e1x_ov2640_read_reg(void *ctx, uint8_t reg, uint8_t *val)
{
    eff_i2c_t *i2c = (eff_i2c_t *)ctx;
    return eff_i2c_read(i2c, OV2640_I2C_ADDR, reg, val, 1u);
}

static int8_t e1x_ov2640_write_reg(void *ctx, uint8_t reg, uint8_t val)
{
    eff_i2c_t *i2c = (eff_i2c_t *)ctx;
    uint8_t data = val;
    return eff_i2c_write(i2c, OV2640_I2C_ADDR, reg, &data, 1u);
}

int main(void)
{
    eff_spi_cfg_t spi_cfg = EFF_SPI_DEFAULTS;
    arducam_spi_bus_t bus;
    arducam_spi_t cam_spi;
    ov2640_bus_t ov_bus;
    ov2640_t ov2640;
    ov2640_id_t ov_id;
    uint8_t arduchip_ver = 0u;

    printf("\r\n=== APP: arducam ===\r\n");
    printf("ArduCAM bring-up on SPI_2 / PINMUX_2 and I2C_4_1 / PINMUX_4\r\n");

    eff_pinmux_set(CAM_SPI_PINMUX, PINMUX_SPI);
    eff_pinmux_set(CAM_I2C_PINMUX, PINMUX_I2C0_I2C1);

    spi_cfg.xfer_mode = SPI_XFER_WRITE_READ;
    spi_cfg.bus_size = SPI_BUS_SINGLE;
    spi_cfg.clk_div = 4;

    if (eff_spi_init(CAM_SPI, &spi_cfg) != 0) {
        printf("SPI init failed\r\n");
        return -1;
    }

    if (eff_i2c_init(CAM_I2C, CAM_I2C_SPEED) != 0) {
        printf("I2C init failed\r\n");
        return -1;
    }

    bus.transact = e1x_spi_transact;
    bus.ctx = CAM_SPI;

    if (arducam_spi_init(&cam_spi, &bus) != ARDUCAM_SPI_OK) {
        printf("ArduCAM SPI wrapper init failed\r\n");
        return -1;
    }

    if (arducam_spi_check_link(&cam_spi) != ARDUCAM_SPI_OK) {
        printf("ArduChip SPI link check failed\r\n");
        return -1;
    }

    printf("ArduChip SPI link check passed\r\n");

    if (arducam_spi_read_reg(&cam_spi, 0x40u, &arduchip_ver) != ARDUCAM_SPI_OK) {
        printf("ArduChip version read failed\r\n");
        return -1;
    }

    printf("ArduChip version reg: 0x%02X\r\n", arduchip_ver);

    ov_bus.read_reg = e1x_ov2640_read_reg;
    ov_bus.write_reg = e1x_ov2640_write_reg;
    ov_bus.ctx = CAM_I2C;

    if (ov2640_init(&ov2640, &ov_bus) != OV2640_OK) {
        printf("OV2640 SCCB wrapper init failed\r\n");
        return -1;
    }

    if (ov2640_read_id(&ov2640, &ov_id) != OV2640_OK) {
        printf("OV2640 ID read failed\r\n");
        return -1;
    }

    printf("OV2640 ID: MIDH=0x%02X MIDL=0x%02X PIDH=0x%02X PIDL=0x%02X\r\n",
           ov_id.midh, ov_id.midl, ov_id.pidh, ov_id.pidl);

    if (ov2640_check_id(&ov2640, &ov_id) != OV2640_OK) {
        printf("OV2640 ID check failed\r\n");
        return -1;
    }

    printf("OV2640 ID check passed\r\n");

    while (1) {
        sleep(1);
    }

    return 0;
}
