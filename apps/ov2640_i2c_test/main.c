#include <eff.h>
#include <eff/drivers/i2c.h>
#include <stdio.h>

#include "../arducam/ov2640_sccb.h"

#define CAM_I2C_PINMUX PINMUX_4
#define CAM_I2C        I2C_4_1
#define CAM_I2C_SPEED  I2C_SPEED_100K

#define OV2640_I2C_ADDR 0x3Cu

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
    ov2640_bus_t ov_bus;
    ov2640_t ov2640;
    ov2640_id_t ov_id;
    int8_t rc;

    sleep_ms(1000);
    printf("\r\n=== APP: ov2640_i2c_test ===\r\n");
    printf("OV2640 I2C test on I2C_4_1 / PINMUX_4\r\n");

    eff_pinmux_set(CAM_I2C_PINMUX, PINMUX_I2C0_I2C1);

    rc = eff_i2c_init(CAM_I2C, CAM_I2C_SPEED);
    printf("eff_i2c_init -> %d\r\n", rc);
    if (rc != 0) {
        return -1;
    }

    ov_bus.read_reg = e1x_ov2640_read_reg;
    ov_bus.write_reg = e1x_ov2640_write_reg;
    ov_bus.ctx = CAM_I2C;

    rc = ov2640_init(&ov2640, &ov_bus);
    printf("ov2640_init -> %d\r\n", rc);
    if (rc != OV2640_OK) {
        return -1;
    }

    rc = ov2640_read_id(&ov2640, &ov_id);
    printf("ov2640_read_id -> %d\r\n", rc);
    if (rc != OV2640_OK) {
        return -1;
    }

    printf("MIDH=0x%02X MIDL=0x%02X PIDH=0x%02X PIDL=0x%02X\r\n",
           ov_id.midh, ov_id.midl, ov_id.pidh, ov_id.pidl);

    rc = ov2640_check_id(&ov2640, &ov_id);
    printf("ov2640_check_id -> %d\r\n", rc);

    while (1) {
        sleep(1);
    }

    return 0;
}
