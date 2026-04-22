#include "arducam_spi.h"

/* ArduChip register map used by the SPI-side camera bridge. */
#define ARDUCHIP_REG_TEST       0x00u
#define ARDUCHIP_REG_FIFO_CTRL  0x04u
#define ARDUCHIP_REG_TRIG       0x41u
#define ARDUCHIP_REG_FIFO_SZ0   0x42u
#define ARDUCHIP_REG_FIFO_SZ1   0x43u
#define ARDUCHIP_REG_FIFO_SZ2   0x44u
#define ARDUCHIP_CMD_BURST_READ 0x3Cu

#define ARDUCHIP_FIFO_CLEAR_DONE  0x01u
#define ARDUCHIP_FIFO_START       0x02u
#define ARDUCHIP_FIFO_CLEAR_WPTR  0x10u
#define ARDUCHIP_FIFO_CLEAR_RPTR  0x20u

#define ARDUCHIP_TRIG_CAP_DONE  0x08u

static int8_t arducam_spi_bus_ok(const arducam_spi_t *cam)
{
    if ((cam == NULL) || (cam->bus.transact == NULL)) {
        return ARDUCAM_SPI_ERR_ARG;
    }
    return ARDUCAM_SPI_OK;
}

int8_t arducam_spi_init(arducam_spi_t *cam, const arducam_spi_bus_t *bus)
{
    if ((cam == NULL) || (bus == NULL) || (bus->transact == NULL)) {
        return ARDUCAM_SPI_ERR_ARG;
    }

    cam->bus = *bus;
    return ARDUCAM_SPI_OK;
}

int8_t arducam_spi_write_reg(const arducam_spi_t *cam, uint8_t addr, uint8_t data)
{
    uint8_t tx[2];

    if (arducam_spi_bus_ok(cam) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_ARG;
    }

    tx[0] = (uint8_t)(addr | 0x80u);
    tx[1] = data;

    return (cam->bus.transact(cam->bus.ctx, tx, 2u, NULL, 0u) == 0) ?
        ARDUCAM_SPI_OK : ARDUCAM_SPI_ERR_IO;
}

int8_t arducam_spi_read_reg(const arducam_spi_t *cam, uint8_t addr, uint8_t *data)
{
    uint8_t tx;

    if ((arducam_spi_bus_ok(cam) != ARDUCAM_SPI_OK) || (data == NULL)) {
        return ARDUCAM_SPI_ERR_ARG;
    }

    tx = (uint8_t)(addr & 0x7Fu);

    return (cam->bus.transact(cam->bus.ctx, &tx, 1u, data, 1u) == 0) ?
        ARDUCAM_SPI_OK : ARDUCAM_SPI_ERR_IO;
}

int8_t arducam_spi_check_link(const arducam_spi_t *cam)
{
    uint8_t value = 0u;

    if (arducam_spi_write_reg(cam, ARDUCHIP_REG_TEST, 0x55u) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }
    if (arducam_spi_read_reg(cam, ARDUCHIP_REG_TEST, &value) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }

    return (value == 0x55u) ? ARDUCAM_SPI_OK : ARDUCAM_SPI_ERR_IO;
}

int8_t arducam_spi_reset_fifo(const arducam_spi_t *cam)
{
    return arducam_spi_write_reg(cam, ARDUCHIP_REG_FIFO_CTRL,
                                 ARDUCHIP_FIFO_CLEAR_DONE |
                                 ARDUCHIP_FIFO_CLEAR_WPTR |
                                 ARDUCHIP_FIFO_CLEAR_RPTR);
}

int8_t arducam_spi_start_capture(const arducam_spi_t *cam)
{
    return arducam_spi_write_reg(cam, ARDUCHIP_REG_FIFO_CTRL, ARDUCHIP_FIFO_START);
}

int8_t arducam_spi_capture_done(const arducam_spi_t *cam, uint8_t *done)
{
    uint8_t trig = 0u;

    if (done == NULL) {
        return ARDUCAM_SPI_ERR_ARG;
    }
    if (arducam_spi_read_reg(cam, ARDUCHIP_REG_TRIG, &trig) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }

    *done = (uint8_t)((trig & ARDUCHIP_TRIG_CAP_DONE) != 0u);
    return ARDUCAM_SPI_OK;
}

int8_t arducam_spi_fifo_length(const arducam_spi_t *cam, uint32_t *len)
{
    uint8_t sz0 = 0u;
    uint8_t sz1 = 0u;
    uint8_t sz2 = 0u;

    if (len == NULL) {
        return ARDUCAM_SPI_ERR_ARG;
    }
    if (arducam_spi_read_reg(cam, ARDUCHIP_REG_FIFO_SZ0, &sz0) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }
    if (arducam_spi_read_reg(cam, ARDUCHIP_REG_FIFO_SZ1, &sz1) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }
    if (arducam_spi_read_reg(cam, ARDUCHIP_REG_FIFO_SZ2, &sz2) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }

    *len = ((uint32_t)sz0) | (((uint32_t)sz1) << 8u) | (((uint32_t)sz2) << 16u);
    return ARDUCAM_SPI_OK;
}

size_t arducam_spi_fifo_burst_read(const arducam_spi_t *cam, uint8_t *buf, size_t len)
{
    uint8_t tx = ARDUCHIP_CMD_BURST_READ;

    if ((arducam_spi_bus_ok(cam) != ARDUCAM_SPI_OK) || ((buf == NULL) && (len != 0u))) {
        return 0u;
    }

    if (cam->bus.transact(cam->bus.ctx, &tx, 1u, buf, (uint32_t)len) != 0) {
        return 0u;
    }

    return len;
}
