#include <eff.h>
#include <eff/drivers/i2c.h>
#include <eff/drivers/spi.h>
#include <stdint.h>
#include <string.h>

#include "arducam_arch_raspberrypi.h"

#define CAM_SPI_PINMUX PINMUX_2
#define CAM_SPI        SPI_2

#define CAM_I2C_PINMUX PINMUX_4
#define CAM_I2C        I2C_4_1
#define CAM_I2C_SPEED  I2C_SPEED_100K

static uint8_t g_sensor_addr = 0x3Cu;

extern "C" void pinMode(int pin, int mode)
{
    (void)pin;
    (void)mode;
}

extern "C" void digitalWrite(int pin, int value)
{
    (void)pin;
    (void)value;
}

extern "C" bool wiring_init(void)
{
    eff_spi_cfg_t spi_cfg = EFF_SPI_DEFAULTS;

    eff_pinmux_set(CAM_SPI_PINMUX, PINMUX_SPI);
    eff_pinmux_set(CAM_I2C_PINMUX, PINMUX_I2C0_I2C1);

    spi_cfg.xfer_mode = SPI_XFER_WRITE_READ;
    spi_cfg.bus_size = SPI_BUS_SINGLE;
    spi_cfg.clk_div = 16;

    if (eff_spi_init(CAM_SPI, &spi_cfg) != 0) {
        return false;
    }

    if (eff_i2c_init(CAM_I2C, CAM_I2C_SPEED) != 0) {
        return false;
    }

    return true;
}

extern "C" bool arducam_i2c_init(uint8_t sensor_addr)
{
    g_sensor_addr = sensor_addr;
    return true;
}

extern "C" void arducam_delay_ms(uint32_t delay)
{
    sleep_ms(delay);
}

extern "C" void arducam_spi_write(uint8_t address, uint8_t value)
{
    uint8_t tx[2] = {address, value};
    uint8_t dummy = 0u;

    (void)eff_spi_xfer(CAM_SPI, 0u, 0u, tx, 2u, &dummy, 1u);
}

extern "C" uint8_t arducam_spi_read(uint8_t address)
{
    uint8_t tx[2] = {address, 0x00u};
    uint8_t rx[2] = {0u, 0u};

    (void)eff_spi_xfer(CAM_SPI, 0u, 0u, tx, 2u, rx, 2u);
    return rx[1];
}

extern "C" void arducam_spi_transfers(uint8_t *buf, uint32_t size)
{
    uint32_t i;
    uint8_t rx[64];

    for (i = 0u; i < size; i += sizeof(rx)) {
        uint32_t chunk = (size - i > sizeof(rx)) ? sizeof(rx) : (size - i);
        memset(rx, 0, chunk);
        (void)eff_spi_xfer(CAM_SPI, 0u, 0u, &buf[i], chunk, rx, chunk);
        memcpy(&buf[i], rx, chunk);
    }
}

extern "C" uint8_t arducam_spi_transfer(uint8_t data)
{
    uint8_t tx = data;
    uint8_t rx = 0u;

    (void)eff_spi_xfer(CAM_SPI, 0u, 0u, &tx, 1u, &rx, 1u);
    return rx;
}

extern "C" uint8_t arducam_i2c_write(uint8_t regID, uint8_t regDat)
{
    uint8_t data = regDat;
    return eff_i2c_write(CAM_I2C, g_sensor_addr, regID, &data, 1u) == 0 ? 1u : 0u;
}

extern "C" uint8_t arducam_i2c_read(uint8_t regID, uint8_t* regDat)
{
    return eff_i2c_read(CAM_I2C, g_sensor_addr, regID, regDat, 1u) == 0 ? 1u : 0u;
}

extern "C" uint8_t arducam_i2c_write16(uint8_t regID, uint16_t regDat)
{
    uint8_t data[2] = {
        (uint8_t)(regDat >> 8),
        (uint8_t)(regDat & 0xFFu)
    };
    return eff_i2c_write(CAM_I2C, g_sensor_addr, regID, data, 2u) == 0 ? 1u : 0u;
}

extern "C" uint8_t arducam_i2c_read16(uint8_t regID, uint16_t* regDat)
{
    uint8_t data[2] = {0u, 0u};
    if (eff_i2c_read(CAM_I2C, g_sensor_addr, regID, data, 2u) != 0) {
        return 0u;
    }
    *regDat = (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
    return 1u;
}

extern "C" uint8_t arducam_i2c_word_write(uint16_t regID, uint8_t regDat)
{
    uint8_t data = regDat;
    return eff_i2c_write_wide(CAM_I2C, g_sensor_addr, regID, &data, 1u) == 0 ? 1u : 0u;
}

extern "C" uint8_t arducam_i2c_word_read(uint16_t regID, uint8_t* regDat)
{
    return eff_i2c_read_wide(CAM_I2C, g_sensor_addr, regID, regDat, 1u) == 0 ? 1u : 0u;
}

extern "C" int arducam_i2c_write_regs(const struct sensor_reg reglist[])
{
    const struct sensor_reg *next = reglist;

    while (!((next->reg == 0xffu) && (next->val == 0xffu))) {
        if (!arducam_i2c_write((uint8_t)next->reg, (uint8_t)next->val)) {
            return 0;
        }
        ++next;
    }

    return 1;
}

extern "C" int arducam_i2c_write_regs16(const struct sensor_reg reglist[])
{
    const struct sensor_reg *next = reglist;

    while (!((next->reg == 0xffu) && (next->val == 0xffffu))) {
        if (!arducam_i2c_write16((uint8_t)next->reg, (uint16_t)next->val)) {
            return 0;
        }
        ++next;
        arducam_delay_ms(1u);
    }

    return 1;
}

extern "C" int arducam_i2c_write_word_regs(const struct sensor_reg reglist[])
{
    const struct sensor_reg *next = reglist;

    while (!((next->reg == 0xffffu) && (next->val == 0xffu))) {
        if (!arducam_i2c_word_write(next->reg, (uint8_t)next->val)) {
            return 0;
        }
        ++next;
        arducam_delay_ms(1u);
    }

    return 1;
}
