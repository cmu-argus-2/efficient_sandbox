#include <eff.h>
#include <eff/drivers/spi.h>
#include <stdio.h>

#include "../arducam/arducam_spi.h"

#define CAM_SPI_PINMUX PINMUX_2
#define CAM_SPI        SPI_2

static void uart_settle(void)
{
    sleep_ms(50);
}

static int8_t e1x_spi_transact(void *ctx,
                               const uint8_t *tx,
                               uint32_t tx_len,
                               uint8_t *rx,
                               uint32_t rx_len)
{
    eff_spi_t *spi = (eff_spi_t *)ctx;
    return eff_spi_xfer(spi, 0u, 0u, (uint8_t *)tx, tx_len, rx, rx_len);
}

int main(void)
{
    eff_spi_cfg_t spi_cfg = EFF_SPI_DEFAULTS;
    arducam_spi_bus_t bus;
    arducam_spi_t cam_spi;
    uint8_t test_reg = 0u;
    uint8_t version = 0u;
    int8_t rc;

    sleep_ms(1000);
    printf("\r\n=== APP: arducam_spi_test ===\r\n");
    uart_settle();
    printf("ArduCAM 5MP Plus SPI test on SPI_2 / PINMUX_2\r\n");
    uart_settle();

    eff_pinmux_set(CAM_SPI_PINMUX, PINMUX_SPI);

    spi_cfg.xfer_mode = SPI_XFER_BIDIRECTIONAL;
    spi_cfg.bus_size = SPI_BUS_SINGLE;
    spi_cfg.clk_div = 16;

    rc = eff_spi_init(CAM_SPI, &spi_cfg);
    printf("eff_spi_init -> %d\r\n", rc);
    uart_settle();
    if (rc != 0) {
        return -1;
    }

    bus.transact = e1x_spi_transact;
    bus.ctx = CAM_SPI;

    rc = arducam_spi_init(&cam_spi, &bus);
    printf("arducam_spi_init -> %d\r\n", rc);
    uart_settle();
    if (rc != ARDUCAM_SPI_OK) {
        return -1;
    }

    rc = arducam_spi_write_reg(&cam_spi, 0x00u, 0x55u);
    printf("arducam_spi_write_reg(0x00, 0x55) -> %d\r\n", rc);
    uart_settle();

    rc = arducam_spi_read_reg(&cam_spi, 0x00u, &test_reg);
    printf("arducam_spi_read_reg(0x00) -> %d, val=0x%02X\r\n", rc, test_reg);
    uart_settle();

    rc = arducam_spi_check_link(&cam_spi);
    printf("arducam_spi_check_link -> %d\r\n", rc);
    uart_settle();
    if (rc != ARDUCAM_SPI_OK) {
        printf("SPI link check failed after raw test-reg probe\r\n");
        uart_settle();
    }

    rc = arducam_spi_read_reg(&cam_spi, 0x40u, &version);
    printf("arducam_spi_read_reg(0x40) -> %d, val=0x%02X\r\n", rc, version);
    uart_settle();
    if (rc != ARDUCAM_SPI_OK) {
        return -1;
    }

    rc = arducam_spi_reset_fifo(&cam_spi);
    printf("arducam_spi_reset_fifo -> %d\r\n", rc);
    uart_settle();

    while (1) {
        sleep(1);
    }

    return 0;
}
