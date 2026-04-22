#ifndef ARDUCAM_SPI_H
#define ARDUCAM_SPI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t (*arducam_spi_transact_fn)(void *ctx,
                                          const uint8_t *tx,
                                          uint32_t tx_len,
                                          uint8_t *rx,
                                          uint32_t rx_len);

typedef struct {
    arducam_spi_transact_fn transact;
    void *ctx;
} arducam_spi_bus_t;

typedef struct {
    arducam_spi_bus_t bus;
} arducam_spi_t;

typedef enum {
    ARDUCAM_SPI_OK = 0,
    ARDUCAM_SPI_ERR_ARG = -1,
    ARDUCAM_SPI_ERR_IO = -2
} arducam_spi_err_t;

int8_t arducam_spi_init(arducam_spi_t *cam, const arducam_spi_bus_t *bus);
int8_t arducam_spi_write_reg(const arducam_spi_t *cam, uint8_t addr, uint8_t data);
int8_t arducam_spi_read_reg(const arducam_spi_t *cam, uint8_t addr, uint8_t *data);
int8_t arducam_spi_check_link(const arducam_spi_t *cam);
int8_t arducam_spi_reset_fifo(const arducam_spi_t *cam);
int8_t arducam_spi_start_capture(const arducam_spi_t *cam);
int8_t arducam_spi_capture_done(const arducam_spi_t *cam, uint8_t *done);
int8_t arducam_spi_fifo_length(const arducam_spi_t *cam, uint32_t *len);
size_t arducam_spi_fifo_burst_read(const arducam_spi_t *cam, uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
