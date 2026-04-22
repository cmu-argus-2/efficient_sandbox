#include "arducam_spi.h"

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
    uint8_t tx[2];
    uint8_t rx[2] = {0u, 0u};

    if ((arducam_spi_bus_ok(cam) != ARDUCAM_SPI_OK) || (data == NULL)) {
        return ARDUCAM_SPI_ERR_ARG;
    }

    /*
     * Datasheet single-read timing:
     *   byte 0: command/address
     *   byte 1: dummy byte on MOSI
     *   byte 1 on MISO: register value
     *
     * Keep this as one continuous 2-byte exchange so the device sees the
     * command phase and data phase under a single CS assertion.
     */
    tx[0] = (uint8_t)(addr & 0x7Fu);
    tx[1] = 0x00u;

    if (cam->bus.transact(cam->bus.ctx, tx, 2u, rx, 2u) != 0) {
        return ARDUCAM_SPI_ERR_IO;
    }

    *data = rx[1];
    return ARDUCAM_SPI_OK;
}

int8_t arducam_spi_write_reset(const arducam_spi_t *cam, uint8_t value)
{
    return arducam_spi_write_reg(cam, ARDUCHIP_RESET, value);
}

int8_t arducam_spi_set_bit(const arducam_spi_t *cam, uint8_t addr, uint8_t bit)
{
    uint8_t value = 0u;

    if (arducam_spi_read_reg(cam, addr, &value) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }

    return arducam_spi_write_reg(cam, addr, (uint8_t)(value | bit));
}

int8_t arducam_spi_clear_bit(const arducam_spi_t *cam, uint8_t addr, uint8_t bit)
{
    uint8_t value = 0u;

    if (arducam_spi_read_reg(cam, addr, &value) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }

    return arducam_spi_write_reg(cam, addr, (uint8_t)(value & (uint8_t)(~bit)));
}

int8_t arducam_spi_get_bit(const arducam_spi_t *cam, uint8_t addr, uint8_t bit, uint8_t *value)
{
    uint8_t reg = 0u;

    if (value == NULL) {
        return ARDUCAM_SPI_ERR_ARG;
    }

    if (arducam_spi_read_reg(cam, addr, &reg) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }

    *value = (uint8_t)(reg & bit);
    return ARDUCAM_SPI_OK;
}

int8_t arducam_spi_check_link(const arducam_spi_t *cam)
{
    uint8_t value = 0u;

    if (arducam_spi_write_reg(cam, ARDUCHIP_TEST1, 0x55u) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }
    if (arducam_spi_read_reg(cam, ARDUCHIP_TEST1, &value) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }

    return (value == 0x55u) ? ARDUCAM_SPI_OK : ARDUCAM_SPI_ERR_IO;
}

int8_t arducam_spi_flush_fifo(const arducam_spi_t *cam)
{
    return arducam_spi_write_reg(cam, ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

int8_t arducam_spi_clear_fifo_flag(const arducam_spi_t *cam)
{
    return arducam_spi_write_reg(cam, ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

int8_t arducam_spi_reset_fifo(const arducam_spi_t *cam)
{
    return arducam_spi_write_reg(cam, ARDUCHIP_FIFO,
                                 (uint8_t)(FIFO_CLEAR_MASK |
                                           FIFO_RDPTR_RST_MASK |
                                           FIFO_WRPTR_RST_MASK));
}

int8_t arducam_spi_start_capture(const arducam_spi_t *cam)
{
    return arducam_spi_write_reg(cam, ARDUCHIP_FIFO, FIFO_START_MASK);
}

int8_t arducam_spi_capture_done(const arducam_spi_t *cam, uint8_t *done)
{
    uint8_t trig = 0u;

    if (done == NULL) {
        return ARDUCAM_SPI_ERR_ARG;
    }
    if (arducam_spi_read_reg(cam, ARDUCHIP_TRIG, &trig) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }

    *done = (uint8_t)((trig & CAP_DONE_MASK) != 0u);
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
    if (arducam_spi_read_reg(cam, FIFO_SIZE1, &sz0) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }
    if (arducam_spi_read_reg(cam, FIFO_SIZE2, &sz1) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }
    if (arducam_spi_read_reg(cam, FIFO_SIZE3, &sz2) != ARDUCAM_SPI_OK) {
        return ARDUCAM_SPI_ERR_IO;
    }

    *len = ((((uint32_t)sz2) & 0x7Fu) << 16u) |
           (((uint32_t)sz1) << 8u) |
           ((uint32_t)sz0);
    *len &= 0x07FFFFu;
    return ARDUCAM_SPI_OK;
}

size_t arducam_spi_fifo_burst_read(const arducam_spi_t *cam, uint8_t *buf, size_t len)
{
    uint8_t tx = BURST_FIFO_READ;

    if ((arducam_spi_bus_ok(cam) != ARDUCAM_SPI_OK) || ((buf == NULL) && (len != 0u))) {
        return 0u;
    }

    if (cam->bus.transact(cam->bus.ctx, &tx, 1u, buf, (uint32_t)len) != 0) {
        return 0u;
    }

    return len;
}
