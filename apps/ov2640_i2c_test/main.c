#include <eff.h>
#include <eff/drivers/i2c.h>
#include <stdio.h>

#include "../arducam/ov5642_sccb.h"

#define CAM_I2C_PINMUX PINMUX_4
#define CAM_I2C        I2C_4_1
#define CAM_I2C_SPEED  I2C_SPEED_100K

typedef struct {
    eff_i2c_t *i2c;
    uint8_t addr;
} ov5642_i2c_ctx_t;

static int8_t e1x_ov5642_read_reg(void *ctx, uint16_t reg, uint8_t *val)
{
    ov5642_i2c_ctx_t *ovctx = (ov5642_i2c_ctx_t *)ctx;
    return eff_i2c_read_wide(ovctx->i2c, ovctx->addr, reg, val, 1u);
}

static int8_t e1x_ov5642_write_reg(void *ctx, uint16_t reg, uint8_t val)
{
    ov5642_i2c_ctx_t *ovctx = (ov5642_i2c_ctx_t *)ctx;
    uint8_t data = val;
    return eff_i2c_write_wide(ovctx->i2c, ovctx->addr, reg, &data, 1u);
}

static void e1x_ov5642_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    sleep_ms(ms);
}

static void dump_reg(ov5642_t *ov5642, uint16_t reg, const char *name)
{
    uint8_t val = 0u;
    int8_t rc = ov5642_read_reg(ov5642, reg, &val);
    printf("read %-12s (0x%04X) -> %d, val=0x%02X\r\n", name, reg, rc, val);
}

static void probe_addr(eff_i2c_t *i2c, uint8_t addr)
{
    ov5642_i2c_ctx_t ovctx;
    ov5642_bus_t ov_bus;
    ov5642_t ov5642;
    ov5642_id_t ov_id;
    int8_t rc;

    ovctx.i2c = i2c;
    ovctx.addr = addr;

    ov_bus.read_reg = e1x_ov5642_read_reg;
    ov_bus.write_reg = e1x_ov5642_write_reg;
    ov_bus.delay_ms = e1x_ov5642_delay_ms;
    ov_bus.ctx = &ovctx;

    printf("\r\n--- Probing I2C addr 0x%02X ---\r\n", addr);

    rc = ov5642_init(&ov5642, &ov_bus);
    printf("ov5642_init -> %d\r\n", rc);
    if (rc != OV5642_OK) {
        return;
    }

    dump_reg(&ov5642, 0x300Au, "CHIP_ID_HIGH");
    dump_reg(&ov5642, 0x300Bu, "CHIP_ID_LOW");
    dump_reg(&ov5642, 0x3100u, "SCCB_ID");

    rc = ov5642_read_id(&ov5642, &ov_id);
    printf("ov5642_read_id -> %d\r\n", rc);
    if (rc == OV5642_OK) {
        printf("CHIP_ID_HIGH=0x%02X CHIP_ID_LOW=0x%02X\r\n",
               ov_id.chip_id_high, ov_id.chip_id_low);
    }

    rc = ov5642_check_id(&ov5642, &ov_id);
    printf("ov5642_check_id -> %d\r\n", rc);
}

int main(void)
{
    int8_t rc;

    sleep_ms(1000);
    printf("\r\n=== APP: ov2640_i2c_test ===\r\n");
    printf("OV5642 I2C test on I2C_4_1 / PINMUX_4\r\n");

    eff_pinmux_set(CAM_I2C_PINMUX, PINMUX_I2C0_I2C1);

    rc = eff_i2c_init(CAM_I2C, CAM_I2C_SPEED);
    printf("eff_i2c_init -> %d\r\n", rc);
    if (rc != 0) {
        return -1;
    }

    probe_addr(CAM_I2C, 0x3Cu);

    while (1) {
        sleep(1);
    }

    return 0;
}
