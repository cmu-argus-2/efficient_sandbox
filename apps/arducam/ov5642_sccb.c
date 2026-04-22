#include "ov5642_sccb.h"

#include <stddef.h>

#define OV5642_REG_CHIP_ID_HIGH 0x300Au
#define OV5642_REG_CHIP_ID_LOW  0x300Bu
#define OV5642_CHIP_ID_HIGH_VAL 0x56u
#define OV5642_CHIP_ID_LOW_VAL  0x42u

static int8_t ov5642_bus_ok(const ov5642_t *dev)
{
    if ((dev == NULL) || (dev->bus.read_reg == NULL) || (dev->bus.write_reg == NULL)) {
        return OV5642_ERR_ARG;
    }
    return OV5642_OK;
}

int8_t ov5642_init(ov5642_t *dev, const ov5642_bus_t *bus)
{
    if ((dev == NULL) || (bus == NULL) ||
        (bus->read_reg == NULL) || (bus->write_reg == NULL)) {
        return OV5642_ERR_ARG;
    }

    dev->bus = *bus;
    return OV5642_OK;
}

int8_t ov5642_read_reg(const ov5642_t *dev, uint16_t reg, uint8_t *val)
{
    if ((ov5642_bus_ok(dev) != OV5642_OK) || (val == NULL)) {
        return OV5642_ERR_ARG;
    }

    return (dev->bus.read_reg(dev->bus.ctx, reg, val) == 0) ? OV5642_OK : OV5642_ERR_IO;
}

int8_t ov5642_write_reg(const ov5642_t *dev, uint16_t reg, uint8_t val)
{
    if (ov5642_bus_ok(dev) != OV5642_OK) {
        return OV5642_ERR_ARG;
    }

    return (dev->bus.write_reg(dev->bus.ctx, reg, val) == 0) ? OV5642_OK : OV5642_ERR_IO;
}

int8_t ov5642_read_id(const ov5642_t *dev, ov5642_id_t *id)
{
    if ((ov5642_bus_ok(dev) != OV5642_OK) || (id == NULL)) {
        return OV5642_ERR_ARG;
    }

    if (ov5642_read_reg(dev, OV5642_REG_CHIP_ID_HIGH, &id->chip_id_high) != OV5642_OK) {
        return OV5642_ERR_IO;
    }
    if (ov5642_read_reg(dev, OV5642_REG_CHIP_ID_LOW, &id->chip_id_low) != OV5642_OK) {
        return OV5642_ERR_IO;
    }

    return OV5642_OK;
}

int8_t ov5642_check_id(const ov5642_t *dev, ov5642_id_t *id)
{
    ov5642_id_t local_id;

    if (id == NULL) {
        id = &local_id;
    }

    if (ov5642_read_id(dev, id) != OV5642_OK) {
        return OV5642_ERR_IO;
    }

    if ((id->chip_id_high != OV5642_CHIP_ID_HIGH_VAL) ||
        (id->chip_id_low != OV5642_CHIP_ID_LOW_VAL)) {
        return OV5642_ERR_ID;
    }

    return OV5642_OK;
}
