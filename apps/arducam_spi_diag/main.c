#include <eff.h>
#include <eff/drivers/spi.h>
#include <eff/atc/atcspi200.h>
#include <stdio.h>

#include "../arducam/arducam_spi.h"

#define CAM_SPI_PINMUX PINMUX_2
#define CAM_SPI        SPI_2

typedef struct {
    int8_t rc;
    uint8_t rx[8];
} spi_diag_result_t;

static void uart_settle(void)
{
    sleep_ms(50);
}

static ATCSPI200_RegDef *spi_regs(eff_spi_t *spi)
{
    return (ATCSPI200_RegDef *)spi->base_address;
}

static void dump_spi_regs(const char *label, eff_spi_t *spi)
{
    ATCSPI200_RegDef *regs = spi_regs(spi);

    printf("%s\r\n", label);
    uart_settle();
    printf("  TRANSFMT=0x%08X DIRECTIO=0x%08X TRANSCTRL=0x%08X\r\n",
           regs->TRANSFMT, regs->DIRECTIO, regs->TRANSCTRL);
    uart_settle();
    printf("  STATUS=0x%08X TIMING=0x%08X CONFIG=0x%08X\r\n",
           regs->STATUS, regs->TIMING, regs->CONFIG);
    uart_settle();
}

static void print_bytes(const char *label, const uint8_t *data, uint32_t len)
{
    uint32_t i;

    printf("%s", label);
    for (i = 0; i < len; ++i) {
        printf("%s0x%02X", (i == 0u) ? "" : " ", data[i]);
    }
    if (len == 0u) {
        printf("<none>");
    }
    printf("\r\n");
    uart_settle();
}

static int8_t spi_set_mode(eff_spi_t *spi, eff_spi_xfer_mode_e mode, uint8_t clk_div)
{
    eff_spi_cfg_t cfg = spi->_cfg;

    cfg.xfer_mode = mode;
    cfg.bus_size = SPI_BUS_SINGLE;
    cfg.clk_div = clk_div;
    return eff_spi_init(spi, &cfg);
}

static spi_diag_result_t run_xfer(const char *label,
                                  eff_spi_t *spi,
                                  eff_spi_xfer_mode_e mode,
                                  const uint8_t *tx,
                                  uint32_t tx_len,
                                  uint32_t rx_len)
{
    spi_diag_result_t result = {
        .rc = -127,
        .rx = {0}
    };
    uint8_t tx_buf[8] = {0};
    uint32_t i;

    if (tx_len > sizeof(tx_buf)) {
        printf("%s\r\n", label);
        uart_settle();
        printf("  tx_len too large for local buffer\r\n");
        uart_settle();
        result.rc = -126;
        return result;
    }

    for (i = 0; i < tx_len; ++i) {
        tx_buf[i] = tx[i];
    }

    printf("%s\r\n", label);
    uart_settle();
    printf("  mode=%u tx_len=%u rx_len=%u\r\n",
           (unsigned)mode, (unsigned)tx_len, (unsigned)rx_len);
    uart_settle();
    print_bytes("  tx=", tx_buf, tx_len);

    result.rc = spi_set_mode(spi, mode, 16u);
    printf("  eff_spi_init -> %d\r\n", result.rc);
    uart_settle();
    if (result.rc != 0) {
        return result;
    }

    dump_spi_regs("  before xfer", spi);
    result.rc = eff_spi_xfer(spi, 0u, 0u, tx_buf, tx_len,
                             (rx_len == 0u) ? NULL : result.rx, rx_len);
    printf("  eff_spi_xfer -> %d\r\n", result.rc);
    uart_settle();
    dump_spi_regs("  after xfer", spi);
    if (rx_len != 0u) {
        print_bytes("  rx=", result.rx, rx_len);
    }

    return result;
}

static spi_diag_result_t run_direct_cs_read(const char *label,
                                            eff_spi_t *spi,
                                            const uint8_t *tx,
                                            uint32_t tx_len,
                                            uint32_t rx_len)
{
    spi_diag_result_t result = {
        .rc = -127,
        .rx = {0}
    };
    ATCSPI200_RegDef *regs = spi_regs(spi);
    uint32_t saved_directio;
    uint8_t tx_buf[8] = {0};
    uint32_t i;

    if (tx_len > sizeof(tx_buf)) {
        printf("%s\r\n", label);
        uart_settle();
        printf("  tx_len too large for local buffer\r\n");
        uart_settle();
        result.rc = -126;
        return result;
    }

    for (i = 0; i < tx_len; ++i) {
        tx_buf[i] = tx[i];
    }

    printf("%s\r\n", label);
    uart_settle();
    print_bytes("  tx=", tx_buf, tx_len);

    result.rc = spi_set_mode(spi, SPI_XFER_WRITE_READ, 16u);
    printf("  eff_spi_init -> %d\r\n", result.rc);
    uart_settle();
    if (result.rc != 0) {
        return result;
    }

    saved_directio = regs->DIRECTIO;
    dump_spi_regs("  before direct-cs assert", spi);
    regs->DIRECTIO = saved_directio |
                     ATCSPI200_DIRECTIO_DIRECTIOEN_MASK |
                     ATCSPI200_DIRECTIO_CS_OE_MASK |
                     ATCSPI200_DIRECTIO_CS_O_MASK;
    regs->DIRECTIO &= ~ATCSPI200_DIRECTIO_CS_O_MASK;
    dump_spi_regs("  after direct-cs assert", spi);

    result.rc = eff_spi_xfer(spi, 0u, 0u, tx_buf, tx_len,
                             (rx_len == 0u) ? NULL : result.rx, rx_len);
    printf("  eff_spi_xfer -> %d\r\n", result.rc);
    uart_settle();
    if (rx_len != 0u) {
        print_bytes("  rx=", result.rx, rx_len);
    }

    regs->DIRECTIO |= ATCSPI200_DIRECTIO_CS_O_MASK;
    dump_spi_regs("  after direct-cs deassert", spi);
    regs->DIRECTIO = saved_directio;
    dump_spi_regs("  restored directio", spi);

    return result;
}

static void print_separator(const char *label)
{
    printf("\r\n--- %s ---\r\n", label);
    uart_settle();
}

int main(void)
{
    eff_spi_cfg_t spi_cfg = EFF_SPI_DEFAULTS;
    spi_diag_result_t read_test;
    spi_diag_result_t read_rev;
    spi_diag_result_t safe_write_test;
    spi_diag_result_t safe_read_test;
    spi_diag_result_t safe_read_rev;
    uint8_t tx_reset_assert[2] = {(uint8_t)(ARDUCHIP_RESET | 0x80u), 0x80u};
    uint8_t tx_reset_release[2] = {(uint8_t)(ARDUCHIP_RESET | 0x80u), 0x00u};
    uint8_t tx_test_write[2] = {(uint8_t)(ARDUCHIP_TEST1 | 0x80u), 0x55u};
    uint8_t tx_test_read[1] = {ARDUCHIP_TEST1};
    uint8_t tx_rev_read[1] = {ARDUCHIP_REV};

    sleep_ms(1000);
    printf("\r\n=== APP: arducam_spi_diag ===\r\n");
    uart_settle();
    printf("Raw SPI controller diagnostic on SPI_2 / PINMUX_2\r\n");
    uart_settle();

    eff_pinmux_set(CAM_SPI_PINMUX, PINMUX_SPI);

    spi_cfg.xfer_mode = SPI_XFER_WRITE_READ;
    spi_cfg.bus_size = SPI_BUS_SINGLE;
    spi_cfg.clk_div = 16;

    printf("initial eff_spi_init -> %d\r\n", eff_spi_init(CAM_SPI, &spi_cfg));
    uart_settle();
    dump_spi_regs("after initial init", CAM_SPI);

    print_separator("baseline zero-length writes");
    (void)run_xfer("reset assert", CAM_SPI, SPI_XFER_WRITE_ONLY,
                   tx_reset_assert, 2u, 0u);
    sleep_ms(100);
    (void)run_xfer("reset release", CAM_SPI, SPI_XFER_WRITE_ONLY,
                   tx_reset_release, 2u, 0u);
    sleep_ms(100);
    (void)run_xfer("write test reg", CAM_SPI, SPI_XFER_WRITE_ONLY,
                   tx_test_write, 2u, 0u);

    read_test = run_xfer("read test reg (WRITE_READ)", CAM_SPI,
                         SPI_XFER_WRITE_READ, tx_test_read, 1u, 1u);
    read_rev = run_xfer("read revision reg (WRITE_READ)", CAM_SPI,
                        SPI_XFER_WRITE_READ, tx_rev_read, 1u, 1u);

    if ((read_test.rc == 0) && (read_rev.rc == 0) &&
        (read_test.rx[0] == 0u) && (read_rev.rx[0] == 0u)) {
        printf("both controller-managed reads returned 0x00; trying direct-CS experiment\r\n");
        uart_settle();
        (void)run_direct_cs_read("read test reg (direct-CS experiment)",
                                 CAM_SPI, tx_test_read, 1u, 1u);
        (void)run_direct_cs_read("read revision reg (direct-CS experiment)",
                                 CAM_SPI, tx_rev_read, 1u, 1u);
    }

    print_separator("non-zero-length write workaround");
    safe_write_test = run_xfer("write test reg (WRITE_READ, dummy-rx)", CAM_SPI,
                               SPI_XFER_WRITE_READ, tx_test_write, 2u, 1u);
    safe_read_test = run_xfer("read test reg after safe write", CAM_SPI,
                              SPI_XFER_WRITE_READ, tx_test_read, 1u, 1u);
    safe_read_rev = run_xfer("read revision reg after safe write", CAM_SPI,
                             SPI_XFER_WRITE_READ, tx_rev_read, 1u, 1u);

    printf("summary: baseline test=0x%02X rev=0x%02X | safe-write rc=%d safe test=0x%02X safe rev=0x%02X\r\n",
           read_test.rx[0], read_rev.rx[0], safe_write_test.rc,
           safe_read_test.rx[0], safe_read_rev.rx[0]);
    uart_settle();

    while (1) {
        sleep(1);
    }

    return 0;
}
