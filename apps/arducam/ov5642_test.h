/**
 * arducam_ov5642.h  —  SPI + I2C driver for ArduCAM-Mini-5MP-Plus (OV5642)
 *
 * Hardware overview
 * -----------------
 * The ArduCAM-Mini-5MP-Plus contains two independent buses:
 *
 *   SPI  —  talks to the ArduChip FPGA bridge.
 *            Controls FIFO capture, reads back image data, and accesses
 *            ArduChip control/status registers.
 *
 *   I2C  —  talks directly to the OV5642 image sensor.
 *            Configures resolution, format, exposure, white-balance, etc.
 *            OV5642 uses 16-bit register addresses and 8-bit data values.
 *            I2C device address (7-bit): 0x3C  (8-bit write address: 0x78)
 *
 * Both buses must be operational for the camera to work.  SPI alone can
 * initialise the ArduChip, but without the I2C sensor init the FIFO will
 * never produce valid image data.
 *
 * Usage
 * -----
 *   arducam_hal_t hal = {
 *       .spi_transact = my_spi_transact,  // SPI transfer function
 *       .i2c_write    = my_i2c_write,     // I2C register write
 *       .i2c_read     = my_i2c_read,      // I2C register read
 *       .delay_ms     = my_delay_ms,
 *       .ctx          = &my_context,
 *   };
 *   arducam_t cam;
 *   arducam_init(&cam, &hal, ARDUCAM_RES_JPEG_320x240);
 *   arducam_capture(&cam, 3000);
 *   uint32_t len = arducam_fifo_length(&cam);
 *   // ... read len bytes via arducam_fifo_read_burst() or arducam_fifo_read_byte()
 */

#ifndef ARDUCAM_OV5642_H
#define ARDUCAM_OV5642_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Error codes
 * ============================================================ */
typedef enum {
    ARDUCAM_OK           =  0,
    ARDUCAM_ERR_HAL      = -1,  /* NULL function pointer in HAL          */
    ARDUCAM_ERR_SPI      = -2,  /* SPI loopback test failed              */
    ARDUCAM_ERR_I2C      = -3,  /* I2C sensor write/read failed          */
    ARDUCAM_ERR_TIMEOUT  = -4,  /* Capture did not complete in time      */
} arducam_err_t;

/* ============================================================
 * Supported JPEG output resolutions
 * ============================================================ */
typedef enum {
    ARDUCAM_RES_JPEG_320x240   = 0,
    ARDUCAM_RES_JPEG_640x480   = 1,
    ARDUCAM_RES_JPEG_1024x768  = 2,
    ARDUCAM_RES_JPEG_1280x960  = 3,
    ARDUCAM_RES_JPEG_1600x1200 = 4,
    ARDUCAM_RES_JPEG_2048x1536 = 5,
    ARDUCAM_RES_JPEG_2592x1944 = 6,
} arducam_res_t;

/* ============================================================
 * HAL callbacks
 *
 * spi_transact  — full-duplex or half-duplex SPI transfer.
 *                 tx_buf / tx_len: bytes to send (may be NULL/0).
 *                 rx_buf / rx_len: bytes to receive (may be NULL/0).
 *                 CS must be asserted for the entire call.
 *                 Returns 0 on success, negative on error.
 *
 * i2c_write     — write one 8-bit value to a 16-bit OV5642 register.
 *                 dev_addr is the 7-bit I2C address (0x3C).
 *                 Returns 0 on success, negative on error.
 *
 * i2c_read      — read one 8-bit value from a 16-bit OV5642 register.
 *                 Returns 0 on success, negative on error.
 *
 * delay_ms      — blocking millisecond delay.
 * ============================================================ */
typedef int8_t (*arducam_spi_transact_fn)(void       *ctx,
                                          const uint8_t *tx_buf, uint32_t tx_len,
                                                uint8_t *rx_buf, uint32_t rx_len);

typedef int8_t (*arducam_i2c_write_fn)(void    *ctx,
                                       uint8_t  dev_addr,
                                       uint16_t reg_addr,
                                       uint8_t  data);

typedef int8_t (*arducam_i2c_read_fn)(void     *ctx,
                                      uint8_t   dev_addr,
                                      uint16_t  reg_addr,
                                      uint8_t  *data);

typedef void   (*arducam_delay_ms_fn)(uint32_t ms);

typedef struct {
    arducam_spi_transact_fn  spi_transact;
    arducam_i2c_write_fn     i2c_write;
    arducam_i2c_read_fn      i2c_read;
    arducam_delay_ms_fn      delay_ms;
    void                    *ctx;
} arducam_hal_t;

/* ============================================================
 * Driver instance  (treat as opaque — initialise via arducam_init)
 * ============================================================ */
typedef struct {
    arducam_hal_t hal;
} arducam_t;

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * arducam_init — initialise ArduChip (SPI) and OV5642 sensor (I2C).
 *
 * Sequence:
 *   1. Verify SPI link with test-register loopback.
 *   2. Reset and flush the ArduChip FIFO.
 *   3. Software-reset the OV5642 sensor over I2C.
 *   4. Load QVGA preview init table (required base for all modes).
 *   5. Load JPEG capture table.
 *   6. Apply resolution-specific scaling registers.
 *
 * Returns ARDUCAM_OK on success, or a negative arducam_err_t on failure.
 */
arducam_err_t arducam_init(arducam_t           *cam,
                           const arducam_hal_t *hal,
                           arducam_res_t        res);

/**
 * arducam_capture — trigger a single-frame JPEG capture into the FIFO.
 *
 * Polls the capture-done flag.  Returns ARDUCAM_ERR_TIMEOUT if the frame
 * does not complete within timeout_ms milliseconds.
 */
arducam_err_t arducam_capture(arducam_t *cam, uint32_t timeout_ms);

/**
 * arducam_fifo_length — return the number of bytes written to the FIFO
 * by the last completed capture.  Call after arducam_capture() succeeds.
 */
uint32_t arducam_fifo_length(arducam_t *cam);

/**
 * arducam_fifo_read_byte — read one byte from the FIFO (single-read mode).
 * Slow but simple; use arducam_fifo_read_burst() for bulk transfers.
 */
uint8_t arducam_fifo_read_byte(arducam_t *cam);

/**
 * arducam_fifo_read_burst — read len bytes from the FIFO into buf.
 * CS is held low for the entire burst by the HAL spi_transact call.
 * Returns the number of bytes actually read (always len on success).
 */
size_t arducam_fifo_read_burst(arducam_t *cam, uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ARDUCAM_OV5642_H */
