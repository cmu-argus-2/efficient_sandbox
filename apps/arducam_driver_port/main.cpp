#include <eff.h>
#include <stdio.h>

#include "ArduCAM.h"
#include "arducam_arch_raspberrypi.h"

static void uart_settle(void)
{
    sleep_ms(50);
}

int main(void)
{
    uint8_t vid = 0u;
    uint8_t pid = 0u;
    uint8_t temp = 0u;
    ArduCAM myCAM(OV5642, 21);

    sleep_ms(1000);
    printf("\r\n=== APP: arducam_driver_port ===\r\n");
    uart_settle();
    printf("ArduCAM direct driver port on SPI_2 / PINMUX_2 and I2C_4_1 / PINMUX_4\r\n");
    uart_settle();

    if (!wiring_init()) {
        printf("wiring_init failed\r\n");
        uart_settle();
        return -1;
    }

    myCAM.write_reg(0x07, 0x80);
    printf("reset assert done\r\n");
    uart_settle();
    sleep_ms(100);

    myCAM.write_reg(0x07, 0x00);
    printf("reset release done\r\n");
    uart_settle();
    sleep_ms(100);

    for (int tries = 0; tries < 3; ++tries) {
        myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
        temp = myCAM.read_reg(ARDUCHIP_TEST1);
        printf("spi probe %d -> 0x%02X\r\n", tries, temp);
        uart_settle();
    }

    myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
    printf("OV5642 chip id -> 0x%02X 0x%02X\r\n", vid, pid);
    uart_settle();

    myCAM.set_format(JPEG);
    myCAM.InitCAM();
    myCAM.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
    myCAM.clear_fifo_flag();
    myCAM.write_reg(ARDUCHIP_FRAMES, 0x00);
    printf("driver init path applied\r\n");
    uart_settle();

    while (1) {
        sleep(1);
    }

    return 0;
}
