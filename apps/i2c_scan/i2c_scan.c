#include <eff.h>
#include <stdio.h>
#include <stdint.h>

static void scan_bus(const char *name, eff_i2c_t *bus) {
    printf("\r\n=== Scanning %s ===\r\n", name);

    uint8_t dummy = 0;
    int found = 0;

    for (uint8_t addr = 0x03; addr < 0x78; addr++) {

        int8_t status = eff_i2c_write_raw(
            bus,
            addr,
            0,
            I2C_REG_NONE,
            &dummy,
            0
        );

        // printf("for addr = 0x%02X, status: %d\r\n", addr, status);

        if (status == 0) {
            printf("Found device at 0x%02X on %s\r\n", addr, name);
            found = 1;
        }
    }

    if (!found) {
        printf("No devices found on %s\r\n", name);
    }
}

// Scans for I2C device connected to level shifted I2C pins on arduino shield headers.
int main(void) {
    printf("Starting I2C scan...\r\n");

    // Route pins to I2C mode
    eff_pinmux_set(PINMUX_4, PINMUX_I2C0_I2C1);

    // Initialize both buses at 100 kHz
    if (eff_i2c_init(I2C_4_0, I2C_SPEED_100K)) {
        printf("I2C_4_0 init failed\r\n");
    }

    if (eff_i2c_init(I2C_4_1, I2C_SPEED_100K)) {
        printf("I2C_4_1 init failed\r\n");
    }

    while (1) {
        scan_bus("I2C_4_0", I2C_4_0);
        scan_bus("I2C_4_1", I2C_4_1);

        printf("\r\nScan complete.\r\n");
        sleep(5);
    }

    return 0;
}