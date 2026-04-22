#include "ov2640_sccb.h"

#include <stddef.h>

/*
 * OV2640 identification registers.
 * Common bring-up sequences select sensor bank (0xFF = 0x01) before reading.
 */
#define OV2640_REG_BANK_SEL 0xFFu
#define OV2640_BANK_SENSOR  0x01u

#define OV2640_REG_MIDH     0x1Cu
#define OV2640_REG_MIDL     0x1Du
#define OV2640_REG_PIDH     0x0Au
#define OV2640_REG_PIDL     0x0Bu

#define OV2640_PIDH_VALUE   0x26u

static int8_t ov2640_bus_ok(const ov2640_t *dev)
{
    if ((dev == NULL) || (dev->bus.read_reg == NULL) || (dev->bus.write_reg == NULL)) {
        return OV2640_ERR_ARG;
    }
    return OV2640_OK;
}

int8_t ov2640_init(ov2640_t *dev, const ov2640_bus_t *bus)
{
    if ((dev == NULL) || (bus == NULL) ||
        (bus->read_reg == NULL) || (bus->write_reg == NULL)) {
        return OV2640_ERR_ARG;
    }

    dev->bus = *bus;
    return OV2640_OK;
}

int8_t ov2640_read_reg(const ov2640_t *dev, uint8_t reg, uint8_t *val)
{
    if ((ov2640_bus_ok(dev) != OV2640_OK) || (val == NULL)) {
        return OV2640_ERR_ARG;
    }

    return (dev->bus.read_reg(dev->bus.ctx, reg, val) == 0) ? OV2640_OK : OV2640_ERR_IO;
}

int8_t ov2640_write_reg(const ov2640_t *dev, uint8_t reg, uint8_t val)
{
    if (ov2640_bus_ok(dev) != OV2640_OK) {
        return OV2640_ERR_ARG;
    }

    return (dev->bus.write_reg(dev->bus.ctx, reg, val) == 0) ? OV2640_OK : OV2640_ERR_IO;
}

int8_t ov2640_read_id(const ov2640_t *dev, ov2640_id_t *id)
{
    if ((ov2640_bus_ok(dev) != OV2640_OK) || (id == NULL)) {
        return OV2640_ERR_ARG;
    }

    if (ov2640_write_reg(dev, OV2640_REG_BANK_SEL, OV2640_BANK_SENSOR) != OV2640_OK) {
        return OV2640_ERR_IO;
    }
    if (ov2640_read_reg(dev, OV2640_REG_MIDH, &id->midh) != OV2640_OK) {
        return OV2640_ERR_IO;
    }
    if (ov2640_read_reg(dev, OV2640_REG_MIDL, &id->midl) != OV2640_OK) {
        return OV2640_ERR_IO;
    }
    if (ov2640_read_reg(dev, OV2640_REG_PIDH, &id->pidh) != OV2640_OK) {
        return OV2640_ERR_IO;
    }
    if (ov2640_read_reg(dev, OV2640_REG_PIDL, &id->pidl) != OV2640_OK) {
        return OV2640_ERR_IO;
    }

    return OV2640_OK;
}

int8_t ov2640_check_id(const ov2640_t *dev, ov2640_id_t *id)
{
    ov2640_id_t local_id;

    if (id == NULL) {
        id = &local_id;
    }

    if (ov2640_read_id(dev, id) != OV2640_OK) {
        return OV2640_ERR_IO;
    }

    /*
     * ArduCAM examples commonly expect PIDH=0x26 and PIDL=0x42.
     * In the field, some users also report PIDL=0x41, so accept either
     * low-byte while keeping the high-byte strict.
     */
    if ((id->pidh != OV2640_PIDH_VALUE) ||
        ((id->pidl != 0x41u) && (id->pidl != 0x42u))) {
        return OV2640_ERR_ID;
    }

    return OV2640_OK;
}
