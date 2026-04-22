#include <eff.h>
#include <eff/drivers/i2c.h>
#include <eff/drivers/spi.h>
#include <stdio.h>

#include "arducam_spi.h"
#include "ov5642_sccb.h"

/* Board-specific E1x wiring for this app. */
#define CAM_SPI_PINMUX PINMUX_2
#define CAM_SPI        SPI_2

#define CAM_I2C_PINMUX PINMUX_4
#define CAM_I2C        I2C_4_1
#define CAM_I2C_SPEED  I2C_SPEED_100K

#define CAM_SENSOR_I2C_ADDR 0x3Cu

static int8_t e1x_spi_transact(void *ctx,
                               const uint8_t *tx,
                               uint32_t tx_len,
                               uint8_t *rx,
                               uint32_t rx_len)
{
    eff_spi_t *spi = (eff_spi_t *)ctx;
    return eff_spi_xfer(spi, 0u, 0u, (uint8_t *)tx, tx_len, rx, rx_len);
}

static int8_t e1x_ov5642_read_reg(void *ctx, uint16_t reg, uint8_t *val)
{
    eff_i2c_t *i2c = (eff_i2c_t *)ctx;
    return eff_i2c_read_wide(i2c, CAM_SENSOR_I2C_ADDR, reg, val, 1u);
}

static int8_t e1x_ov5642_write_reg(void *ctx, uint16_t reg, uint8_t val)
{
    eff_i2c_t *i2c = (eff_i2c_t *)ctx;
    uint8_t data = val;
    return eff_i2c_write_wide(i2c, CAM_SENSOR_I2C_ADDR, reg, &data, 1u);
}

static void e1x_ov5642_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    sleep_ms(ms);
}

int main(void)
{
    eff_spi_cfg_t spi_cfg = EFF_SPI_DEFAULTS;
    arducam_spi_bus_t bus;
    arducam_spi_t cam_spi;
    ov5642_bus_t ov_bus;
    ov5642_t ov5642;
    ov5642_id_t ov_id;
    uint8_t arduchip_ver = 0u;

    sleep_ms(1000);
    printf("\r\n=== APP: arducam ===\r\n");
    printf("ArduCAM 5MP Plus bring-up on SPI_2 / PINMUX_2 and I2C_4_1 / PINMUX_4\r\n");

    eff_pinmux_set(CAM_SPI_PINMUX, PINMUX_SPI);
    eff_pinmux_set(CAM_I2C_PINMUX, PINMUX_I2C0_I2C1);

    spi_cfg.xfer_mode = SPI_XFER_BIDIRECTIONAL;
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

    if (arducam_spi_write_reset(&cam_spi, 0x80u) != ARDUCAM_SPI_OK) {
        printf("ArduChip reset assert failed\r\n");
        return -1;
    }
    sleep_ms(100);

    if (arducam_spi_write_reset(&cam_spi, 0x00u) != ARDUCAM_SPI_OK) {
        printf("ArduChip reset release failed\r\n");
        return -1;
    }
    sleep_ms(100);

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

    ov_bus.read_reg = e1x_ov5642_read_reg;
    ov_bus.write_reg = e1x_ov5642_write_reg;
    ov_bus.delay_ms = e1x_ov5642_delay_ms;
    ov_bus.ctx = CAM_I2C;

    if (ov5642_init(&ov5642, &ov_bus) != OV5642_OK) {
        printf("OV5642 SCCB wrapper init failed\r\n");
        return -1;
    }

    if (ov5642_read_id(&ov5642, &ov_id) != OV5642_OK) {
        printf("OV5642 ID read failed\r\n");
        return -1;
    }

    printf("OV5642 ID: CHIP_ID_HIGH=0x%02X CHIP_ID_LOW=0x%02X\r\n",
           ov_id.chip_id_high, ov_id.chip_id_low);

    if (ov5642_check_id(&ov5642, &ov_id) != OV5642_OK) {
        printf("OV5642 ID check failed\r\n");
        return -1;
    }

    printf("OV5642 ID check passed\r\n");

    if (ov5642_init_jpeg(&ov5642, OV5642_SIZE_320X240) != OV5642_OK) {
        printf("OV5642 JPEG init failed\r\n");
        return -1;
    }

    printf("OV5642 JPEG init applied\r\n");

    if (arducam_spi_set_bit(&cam_spi, ARDUCHIP_TIM, VSYNC_LEVEL_MASK) != ARDUCAM_SPI_OK) {
        printf("ArduChip VSYNC config failed\r\n");
        return -1;
    }

    if (arducam_spi_clear_fifo_flag(&cam_spi) != ARDUCAM_SPI_OK) {
        printf("ArduChip clear FIFO flag failed\r\n");
        return -1;
    }

    if (arducam_spi_write_reg(&cam_spi, ARDUCHIP_FRAMES, 0x00u) != ARDUCAM_SPI_OK) {
        printf("ArduChip frame-count config failed\r\n");
        return -1;
    }

    printf("ArduChip timing/frame setup applied\r\n");

    while (1) {
        sleep(1);
    }

    return 0;
}
