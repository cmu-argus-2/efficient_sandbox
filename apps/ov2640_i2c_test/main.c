#include <eff.h>
#include <eff/drivers/i2c.h>
#include <stdio.h>

#include "../arducam/ov2640_sccb.h"

#define CAM_I2C_PINMUX PINMUX_4
#define CAM_I2C        I2C_4_1
#define CAM_I2C_SPEED  I2C_SPEED_100K

typedef struct {
    eff_i2c_t *i2c;
    uint8_t addr;
} ov2640_i2c_ctx_t;

static int8_t e1x_ov2640_read_reg(void *ctx, uint8_t reg, uint8_t *val)
{
    ov2640_i2c_ctx_t *ovctx = (ov2640_i2c_ctx_t *)ctx;
    return eff_i2c_read(ovctx->i2c, ovctx->addr, reg, val, 1u);
}

static int8_t e1x_ov2640_write_reg(void *ctx, uint8_t reg, uint8_t val)
{
    ov2640_i2c_ctx_t *ovctx = (ov2640_i2c_ctx_t *)ctx;
    uint8_t data = val;
    return eff_i2c_write(ovctx->i2c, ovctx->addr, reg, &data, 1u);
}

static void dump_reg(ov2640_t *ov2640, uint8_t reg, const char *name)
{
    uint8_t val = 0u;
    int8_t rc = ov2640_read_reg(ov2640, reg, &val);
    printf("read %-4s (0x%02X) -> %d, val=0x%02X\r\n", name, reg, rc, val);
}

static void probe_addr(eff_i2c_t *i2c, uint8_t addr)
{
    ov2640_i2c_ctx_t ovctx;
    ov2640_bus_t ov_bus;
    ov2640_t ov2640;
    ov2640_id_t ov_id;
    int8_t rc;

    ovctx.i2c = i2c;
    ovctx.addr = addr;

    ov_bus.read_reg = e1x_ov2640_read_reg;
    ov_bus.write_reg = e1x_ov2640_write_reg;
    ov_bus.ctx = &ovctx;

    printf("\r\n--- Probing I2C addr 0x%02X ---\r\n", addr);

    rc = ov2640_init(&ov2640, &ov_bus);
    printf("ov2640_init -> %d\r\n", rc);
    if (rc != OV2640_OK) {
        return;
    }

    rc = ov2640_write_reg(&ov2640, 0xFFu, 0x01u);
    printf("ov2640_write_reg(BANK_SEL=0x01) -> %d\r\n", rc);

    dump_reg(&ov2640, 0xFFu, "BANK");
    dump_reg(&ov2640, 0x1Cu, "MIDH");
    dump_reg(&ov2640, 0x1Du, "MIDL");
    dump_reg(&ov2640, 0x0Au, "PIDH");
    dump_reg(&ov2640, 0x0Bu, "PIDL");

    rc = ov2640_read_id(&ov2640, &ov_id);
    printf("ov2640_read_id -> %d\r\n", rc);
    if (rc == OV2640_OK) {
        printf("MIDH=0x%02X MIDL=0x%02X PIDH=0x%02X PIDL=0x%02X\r\n",
               ov_id.midh, ov_id.midl, ov_id.pidh, ov_id.pidl);
    }

    rc = ov2640_check_id(&ov2640, &ov_id);
    printf("ov2640_check_id -> %d\r\n", rc);
}

int main(void)
{
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

    probe_addr(CAM_I2C, 0x30u);
    probe_addr(CAM_I2C, 0x3Cu);

    while (1) {
        sleep(1);
    }

    return 0;
}
