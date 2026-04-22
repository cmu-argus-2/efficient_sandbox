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

enum {
    ARDUCHIP_TEST1 = 0x00u,
    ARDUCHIP_FRAMES = 0x01u,
    ARDUCHIP_MODE = 0x02u,
    ARDUCHIP_TIM = 0x03u,
    ARDUCHIP_FIFO = 0x04u,
    ARDUCHIP_GPIO = 0x06u,
    ARDUCHIP_RESET = 0x07u,
    ARDUCHIP_REV = 0x40u,
    ARDUCHIP_TRIG = 0x41u,
    FIFO_SIZE1 = 0x42u,
    FIFO_SIZE2 = 0x43u,
    FIFO_SIZE3 = 0x44u,
    BURST_FIFO_READ = 0x3Cu,
    SINGLE_FIFO_READ = 0x3Du,
    VSYNC_LEVEL_MASK = 0x02u,
    FIFO_CLEAR_MASK = 0x01u,
    FIFO_START_MASK = 0x02u,
    FIFO_RDPTR_RST_MASK = 0x10u,
    FIFO_WRPTR_RST_MASK = 0x20u,
    GPIO_RESET_MASK = 0x01u,
    GPIO_PWDN_MASK = 0x02u,
    GPIO_PWREN_MASK = 0x04u,
    CAP_DONE_MASK = 0x08u
};

int8_t arducam_spi_init(arducam_spi_t *cam, const arducam_spi_bus_t *bus);
int8_t arducam_spi_write_reg(const arducam_spi_t *cam, uint8_t addr, uint8_t data);
int8_t arducam_spi_read_reg(const arducam_spi_t *cam, uint8_t addr, uint8_t *data);
int8_t arducam_spi_write_reset(const arducam_spi_t *cam, uint8_t value);
int8_t arducam_spi_set_bit(const arducam_spi_t *cam, uint8_t addr, uint8_t bit);
int8_t arducam_spi_clear_bit(const arducam_spi_t *cam, uint8_t addr, uint8_t bit);
int8_t arducam_spi_get_bit(const arducam_spi_t *cam, uint8_t addr, uint8_t bit, uint8_t *value);
int8_t arducam_spi_check_link(const arducam_spi_t *cam);
int8_t arducam_spi_flush_fifo(const arducam_spi_t *cam);
int8_t arducam_spi_clear_fifo_flag(const arducam_spi_t *cam);
int8_t arducam_spi_reset_fifo(const arducam_spi_t *cam);
int8_t arducam_spi_start_capture(const arducam_spi_t *cam);
int8_t arducam_spi_capture_done(const arducam_spi_t *cam, uint8_t *done);
int8_t arducam_spi_fifo_length(const arducam_spi_t *cam, uint32_t *len);
size_t arducam_spi_fifo_burst_read(const arducam_spi_t *cam, uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
