/**
 * arducam_ov5642.c  —  SPI + I2C driver for ArduCAM-Mini-5MP-Plus (OV5642)
 *
 * See arducam_ov5642.h for hardware overview and usage.
 *
 * Bus split
 * ---------
 *   SPI  →  ArduChip FPGA (FIFO control, capture trigger, status)
 *   I2C  →  OV5642 image sensor (resolution, format, all image settings)
 *
 * Key differences from the OV2640 driver
 * ----------------------------------------
 *   1. Sensor I2C address is 0x3C (7-bit) / 0x78 (8-bit write) vs 0x30/0x60.
 *   2. OV5642 register addresses are 16-bit; OV2640 uses 8-bit addresses.
 *   3. Init sequence is: soft-reset → QVGA preview base → JPEG capture base
 *      → resolution scaling table.  The OV2640 uses a simpler two-step init.
 *   4. JPEG quality tweak register (0x4407) differs between Mini-5MP and
 *      Mini-5MP-Plus boards; this driver targets the Plus variant (0x08).
 *   5. Resolution table covers up to 2592×1944 (QSXGA), vs UXGA on OV2640.
 */

#include "arducam_ov5642.h"

/* ============================================================
 * ArduChip (FPGA bridge) SPI register map
 *
 * Write : TX = [addr | 0x80, data]        RX = ignored
 * Read  : TX = [addr & 0x7F]              RX = [val]
 * Burst : TX = [0x3C]  then RX = [b0…bN] (CS held low throughout)
 * ============================================================ */
#define AC_REG_TEST          0x00u  /* Loopback: write 0x55, read back        */
#define AC_REG_FRAMES        0x01u  /* Number of frames to capture            */
#define AC_REG_MODE          0x02u  /* Capture / streaming mode               */
#define AC_REG_TIMING        0x03u  /* HSYNC / VSYNC / PCLK polarity          */
#define AC_REG_FIFO_CTRL     0x04u  /* FIFO control (see bits below)          */
#define AC_REG_GPIO_DIR      0x05u  /* GPIO direction (0=in, 1=out)           */
#define AC_REG_GPIO_DATA     0x06u  /* GPIO read / write                      */

/* AC_REG_FIFO_CTRL bit masks */
#define AC_FIFO_CLEAR_WR_DONE  0x01u  /* Clear write-done flag                */
#define AC_FIFO_START_CAPTURE  0x02u  /* Start one-frame capture              */
#define AC_FIFO_CLEAR_RPTR     0x10u  /* Reset FIFO read pointer              */
#define AC_FIFO_CLEAR_WPTR     0x20u  /* Reset FIFO write pointer             */

/* Trigger / status register */
#define AC_REG_TRIG          0x41u
#define AC_TRIG_VSYNC        0x01u  /* VSYNC is active                        */
#define AC_TRIG_SHUTTER      0x02u  /* Shutter button pressed                 */
#define AC_TRIG_CAP_DONE     0x08u  /* Frame fully written to FIFO            */

/* FIFO data access commands */
#define AC_CMD_BURST_READ    0x3Cu  /* Issue once; hold CS low; clock data out */
#define AC_CMD_SINGLE_READ   0x3Du  /* One byte per CS assertion              */

/* FIFO byte-count registers (24-bit, little-endian across three registers) */
#define AC_REG_FIFO_SZ0      0x42u  /* bits  [7:0]  */
#define AC_REG_FIFO_SZ1      0x43u  /* bits [15:8]  */
#define AC_REG_FIFO_SZ2      0x44u  /* bits [22:16] */

/* ============================================================
 * OV5642 I2C sensor constants
 *
 * The OV5642 uses 16-bit register addresses and 8-bit data.
 * I2C 7-bit address: 0x3C  (8-bit write address: 0x78)
 *
 * This is different from the OV2640 which uses 8-bit register
 * addresses and a 7-bit I2C address of 0x30 (8-bit write: 0x60).
 * ============================================================ */
#define OV5642_I2C_ADDR      0x3Cu  /* 7-bit I2C address                      */

/* OV5642 chip ID registers — used to verify I2C communication */
#define OV5642_CHIPID_HIGH   0x300Au /* Should read 0x56                      */
#define OV5642_CHIPID_LOW    0x300Bu /* Should read 0x42                      */

/* Software reset register */
#define OV5642_REG_RESET     0x3008u
#define OV5642_RESET_VAL     0x80u

/* Sentinel that marks the end of a register table */
#define REG_END_ADDR         0xFFFFu
#define REG_END_VAL          0xFFu

/* ============================================================
 * OV5642 register tables
 *
 * Format: { uint16_t reg, uint8_t val } pairs terminated by
 * { 0xFFFF, 0xFF }.
 *
 * Derived from ArduCAM reference firmware (ArduCAM.cpp / ov5642_regs.h).
 * ============================================================ */

typedef struct { uint16_t reg; uint8_t val; } reg16_val8_t;

/* ------------------------------------------------------------------
 * Step 1 — OV5642 QVGA preview base init
 *
 * This large table configures the full sensor pipeline.  It must be
 * loaded first before any resolution-specific tables.  A 100 ms delay
 * is required after loading before proceeding to the JPEG table.
 * ------------------------------------------------------------------ */
static const reg16_val8_t OV5642_QVGA_Preview[] = {
    {0x3103, 0x93}, {0x3008, 0x82}, {0x3017, 0x7f}, {0x3018, 0xfc},
    {0x3810, 0xc2}, {0x3615, 0xf0}, {0x3000, 0x00}, {0x3001, 0x00},
    {0x3002, 0x5c}, {0x3003, 0x00}, {0x3004, 0xff}, {0x3005, 0xff},
    {0x3006, 0x43}, {0x3007, 0x37}, {0x3011, 0x08}, {0x3010, 0x10},
    {0x460c, 0x22}, {0x3815, 0x04}, {0x370c, 0xa0}, {0x3602, 0xfc},
    {0x3612, 0xff}, {0x3634, 0xc0}, {0x3613, 0x00}, {0x3605, 0x7c},
    {0x3621, 0x09}, {0x3622, 0x60}, {0x3604, 0x40}, {0x3603, 0xa7},
    {0x3603, 0x27}, {0x4000, 0x21}, {0x401d, 0x22}, {0x3600, 0x54},
    {0x3605, 0x04}, {0x3606, 0x3f}, {0x3c01, 0x80}, {0x5000, 0x4f},
    {0x5020, 0x04}, {0x5181, 0x79}, {0x5182, 0x00}, {0x5185, 0x22},
    {0x5197, 0x01}, {0x5500, 0x0a}, {0x5504, 0x00}, {0x5505, 0x7f},
    {0x5080, 0x08}, {0x300e, 0x18}, {0x4610, 0x00}, {0x471d, 0x05},
    {0x4708, 0x06}, {0x3710, 0x10}, {0x3632, 0x41}, {0x3702, 0x40},
    {0x3620, 0x37}, {0x3631, 0x01}, {0x370c, 0xa0}, {0x3704, 0xa1},
    {0x370a, 0x11}, {0x3700, 0xa0}, {0x3705, 0x1a}, {0x370e, 0x0e},
    {0x3706, 0x9c}, {0x3712, 0x0c}, {0x3713, 0x0c}, {0x3715, 0x01},
    {0x3717, 0x06}, {0x3718, 0x00}, {0x3719, 0x00}, {0x371c, 0x20},
    {0x3870, 0x00}, {0x3811, 0x00}, {0x3812, 0x00}, {0x3800, 0x00},
    {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x00}, {0x3804, 0x0a},
    {0x3805, 0x20}, {0x3806, 0x07}, {0x3807, 0x98}, {0x3808, 0x0a},
    {0x3809, 0x20}, {0x380a, 0x07}, {0x380b, 0x98}, {0x380c, 0x0c},
    {0x380d, 0x80}, {0x380e, 0x07}, {0x380f, 0xd0}, {0x3810, 0xc2},
    {0x3811, 0x00}, {0x3812, 0x00}, {0x3708, 0x64}, {0x4001, 0x02},
    {0x4005, 0x1a}, {0x3000, 0x00}, {0x3001, 0x00}, {0x3002, 0x00},
    {0x3003, 0x00}, {0x3010, 0x00}, {0x300e, 0x08}, {0x460b, 0x35},
    {0x471d, 0x00}, {0x3815, 0x01}, {0x3818, 0xc0}, {0x501f, 0x00},
    {0x5002, 0xe0}, {0x4300, 0x30}, {0x4300, 0x30}, {0x460b, 0x35},
    {0x460c, 0x22}, {0x4713, 0x03}, {0x4407, 0x04}, {0x440e, 0x00},
    {0x460b, 0x37}, {0x3824, 0x01}, {0x5001, 0x00},
    {0x5000, 0x00}, {0x5001, 0xff}, {0x5000, 0xcf},
    {REG_END_ADDR, REG_END_VAL}
};

/* ------------------------------------------------------------------
 * Step 2 — JPEG capture QSXGA base table
 *
 * Configures the full-resolution JPEG pipeline.  Load after
 * OV5642_QVGA_Preview with a 100 ms gap before and after.
 * ------------------------------------------------------------------ */
static const reg16_val8_t OV5642_JPEG_Capture_QSXGA[] = {
    {0x3a00, 0x78}, {0x3a1a, 0x04}, {0x3a13, 0x30}, {0x3a18, 0x00},
    {0x3a19, 0x7c}, {0x3a08, 0x12}, {0x3a09, 0xc0}, {0x3a0a, 0x0f},
    {0x3a0b, 0xa0}, {0x3004, 0xff}, {0x350c, 0x07}, {0x350d, 0xd0},
    {0x3a0d, 0x08}, {0x3a0e, 0x06}, {0x3500, 0x00}, {0x3501, 0x00},
    {0x3502, 0x00}, {0x350a, 0x00}, {0x350b, 0x00}, {0x3503, 0x00},
    {0x3030, 0x2b}, {0x3a02, 0x00}, {0x3a03, 0x7d}, {0x3a04, 0x00},
    {0x3a14, 0x00}, {0x3a15, 0x7d}, {0x3a16, 0x00}, {0x3a00, 0x78},
    {0x3a08, 0x09}, {0x3a09, 0x60}, {0x3a0a, 0x07}, {0x3a0b, 0xd0},
    {0x3a0d, 0x10}, {0x3a0e, 0x0d}, {0x4407, 0x08},
    /* JPEG stream config */
    {0x460b, 0x37}, {0x471d, 0x05}, {0x4713, 0x02}, {0x3824, 0x01},
    {0x4300, 0x30}, {0x4601, 0x06}, {0x471d, 0x04},
    {REG_END_ADDR, REG_END_VAL}
};

/* ------------------------------------------------------------------
 * Resolution scaling tables  (applied after JPEG capture base)
 *
 * These set the output window registers for the desired JPEG size.
 * The sensor always captures full QSXGA; the ISP scales down.
 * ------------------------------------------------------------------ */

/* 320 × 240 */
static const reg16_val8_t OV5642_320x240[] = {
    {0x3800, 0x01}, {0x3801, 0xa8}, {0x3802, 0x00}, {0x3803, 0x0a},
    {0x3804, 0x0a}, {0x3805, 0x20}, {0x3806, 0x07}, {0x3807, 0x98},
    {0x3808, 0x01}, {0x3809, 0x40}, {0x380a, 0x00}, {0x380b, 0xf0},
    {0x380c, 0x0c}, {0x380d, 0x80}, {0x380e, 0x07}, {0x380f, 0xd0},
    {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0a},
    {0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07},
    {0x5687, 0x98}, {0x3801, 0xb0},
    {REG_END_ADDR, REG_END_VAL}
};

/* 640 × 480 */
static const reg16_val8_t OV5642_640x480[] = {
    {0x3800, 0x01}, {0x3801, 0xa8}, {0x3802, 0x00}, {0x3803, 0x0a},
    {0x3804, 0x0a}, {0x3805, 0x20}, {0x3806, 0x07}, {0x3807, 0x98},
    {0x3808, 0x02}, {0x3809, 0x80}, {0x380a, 0x01}, {0x380b, 0xe0},
    {0x380c, 0x0c}, {0x380d, 0x80}, {0x380e, 0x07}, {0x380f, 0xd0},
    {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0a},
    {0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07},
    {0x5687, 0x98}, {0x3801, 0xb0},
    {REG_END_ADDR, REG_END_VAL}
};

/* 1024 × 768 */
static const reg16_val8_t OV5642_1024x768[] = {
    {0x3800, 0x01}, {0x3801, 0x50}, {0x3802, 0x01}, {0x3803, 0xb2},
    {0x3804, 0x08}, {0x3805, 0xef}, {0x3806, 0x05}, {0x3807, 0xf1},
    {0x3808, 0x04}, {0x3809, 0x00}, {0x380a, 0x03}, {0x380b, 0x00},
    {0x380c, 0x07}, {0x380d, 0x64}, {0x380e, 0x04}, {0x380f, 0x12},
    {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0a},
    {0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07},
    {0x5687, 0x98},
    {REG_END_ADDR, REG_END_VAL}
};

/* 1280 × 960 */
static const reg16_val8_t OV5642_1280x960[] = {
    {0x3800, 0x01}, {0x3801, 0x50}, {0x3802, 0x01}, {0x3803, 0xb2},
    {0x3804, 0x08}, {0x3805, 0xef}, {0x3806, 0x05}, {0x3807, 0xf1},
    {0x3808, 0x05}, {0x3809, 0x00}, {0x380a, 0x03}, {0x380b, 0xc0},
    {0x380c, 0x07}, {0x380d, 0x64}, {0x380e, 0x04}, {0x380f, 0x12},
    {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0a},
    {0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07},
    {0x5687, 0x98},
    {REG_END_ADDR, REG_END_VAL}
};

/* 1600 × 1200 */
static const reg16_val8_t OV5642_1600x1200[] = {
    {0x3800, 0x01}, {0x3801, 0x50}, {0x3802, 0x01}, {0x3803, 0xb2},
    {0x3804, 0x08}, {0x3805, 0xef}, {0x3806, 0x05}, {0x3807, 0xf1},
    {0x3808, 0x06}, {0x3809, 0x40}, {0x380a, 0x04}, {0x380b, 0xb0},
    {0x380c, 0x07}, {0x380d, 0x64}, {0x380e, 0x06}, {0x380f, 0x00},
    {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0a},
    {0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07},
    {0x5687, 0x98},
    {REG_END_ADDR, REG_END_VAL}
};

/* 2048 × 1536 */
static const reg16_val8_t OV5642_2048x1536[] = {
    {0x3800, 0x01}, {0x3801, 0x50}, {0x3802, 0x01}, {0x3803, 0xb2},
    {0x3804, 0x08}, {0x3805, 0xef}, {0x3806, 0x05}, {0x3807, 0xf1},
    {0x3808, 0x08}, {0x3809, 0x00}, {0x380a, 0x06}, {0x380b, 0x00},
    {0x380c, 0x0c}, {0x380d, 0x80}, {0x380e, 0x07}, {0x380f, 0xd0},
    {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0a},
    {0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07},
    {0x5687, 0x98},
    {REG_END_ADDR, REG_END_VAL}
};

/* 2592 × 1944  (native full QSXGA) */
static const reg16_val8_t OV5642_2592x1944[] = {
    {0x3800, 0x01}, {0x3801, 0x50}, {0x3802, 0x01}, {0x3803, 0xb2},
    {0x3804, 0x08}, {0x3805, 0xef}, {0x3806, 0x05}, {0x3807, 0xf1},
    {0x3808, 0x0a}, {0x3809, 0x20}, {0x380a, 0x07}, {0x380b, 0x98},
    {0x380c, 0x0c}, {0x380d, 0x80}, {0x380e, 0x07}, {0x380f, 0xd0},
    {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0a},
    {0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07},
    {0x5687, 0x98},
    {REG_END_ADDR, REG_END_VAL}
};

/* Resolution lookup table — indexed by arducam_res_t */
static const reg16_val8_t * const RES_TABLE[] = {
    OV5642_320x240,    /* ARDUCAM_RES_JPEG_320x240   */
    OV5642_640x480,    /* ARDUCAM_RES_JPEG_640x480   */
    OV5642_1024x768,   /* ARDUCAM_RES_JPEG_1024x768  */
    OV5642_1280x960,   /* ARDUCAM_RES_JPEG_1280x960  */
    OV5642_1600x1200,  /* ARDUCAM_RES_JPEG_1600x1200 */
    OV5642_2048x1536,  /* ARDUCAM_RES_JPEG_2048x1536 */
    OV5642_2592x1944,  /* ARDUCAM_RES_JPEG_2592x1944 */
};

#define RES_TABLE_LEN  ((uint32_t)(sizeof(RES_TABLE) / sizeof(RES_TABLE[0])))

/* ============================================================
 * Internal helpers — ArduChip SPI register access
 *
 * Write : send [addr | 0x80, data] as a 2-byte transaction
 * Read  : send [addr & 0x7F] then receive 1 byte
 * ============================================================ */

static void ac_write_reg(const arducam_t *cam, uint8_t addr, uint8_t data)
{
    uint8_t tx[2] = {(uint8_t)(addr | 0x80u), data};
    cam->hal.spi_transact(cam->hal.ctx, tx, 2u, NULL, 0u);
}

static uint8_t ac_read_reg(const arducam_t *cam, uint8_t addr)
{
    uint8_t tx = (uint8_t)(addr & 0x7Fu);
    uint8_t rx = 0u;
    cam->hal.spi_transact(cam->hal.ctx, &tx, 1u, &rx, 1u);
    return rx;
}

/* ============================================================
 * Internal helpers — OV5642 sensor I2C access
 *
 * The OV5642 uses 16-bit register addresses, 8-bit data.
 * I2C 7-bit device address: OV5642_I2C_ADDR (0x3C).
 *
 * This is the key difference from the OV2640 driver, which uses
 * 8-bit register addresses via the ArduChip SCCB bridge over SPI.
 * Here we talk to the OV5642 directly via a real I2C peripheral.
 * ============================================================ */

static int sensor_write(const arducam_t *cam, uint16_t reg, uint8_t val)
{
    return (int)cam->hal.i2c_write(cam->hal.ctx, OV5642_I2C_ADDR, reg, val);
}

static int sensor_read(const arducam_t *cam, uint16_t reg, uint8_t *val)
{
    return (int)cam->hal.i2c_read(cam->hal.ctx, OV5642_I2C_ADDR, reg, val);
}

/**
 * Walk a {reg16, val8} table and write each entry via I2C.
 * Returns 0 on success, -1 on the first I2C fault.
 */
static int sensor_load_table(const arducam_t *cam, const reg16_val8_t *tbl)
{
    while (!((tbl->reg == REG_END_ADDR) && (tbl->val == REG_END_VAL))) {
        if (sensor_write(cam, tbl->reg, tbl->val) != 0) {
            return -1;
        }
        ++tbl;
    }
    return 0;
}

/* ============================================================
 * Public API
 * ============================================================ */

arducam_err_t arducam_init(arducam_t           *cam,
                           const arducam_hal_t *hal,
                           arducam_res_t        res)
{
    uint8_t chip_id_high = 0u;
    uint8_t chip_id_low  = 0u;

    /* Validate HAL */
    if ((cam == NULL) || (hal == NULL)) {
        return ARDUCAM_ERR_HAL;
    }
    if ((hal->spi_transact == NULL) ||
        (hal->i2c_write    == NULL) ||
        (hal->i2c_read     == NULL) ||
        (hal->delay_ms     == NULL)) {
        return ARDUCAM_ERR_HAL;
    }

    cam->hal = *hal;

    /* ---- 1. Verify SPI link (ArduChip test register loopback) ---------- */
    ac_write_reg(cam, AC_REG_TEST, 0x55u);
    if (ac_read_reg(cam, AC_REG_TEST) != 0x55u) {
        return ARDUCAM_ERR_SPI;
    }

    /* ---- 2. Reset and flush ArduChip FIFO ------------------------------ */
    ac_write_reg(cam, AC_REG_FIFO_CTRL,
                 AC_FIFO_CLEAR_WR_DONE | AC_FIFO_CLEAR_RPTR | AC_FIFO_CLEAR_WPTR);

    /* ---- 3. OV5642 software reset via I2C ------------------------------ */
    /*
     * Unlike the OV2640 (which is reset via the ArduChip SCCB bridge over
     * SPI), the OV5642 is reset directly via I2C.  Register 0x3008 = 0x80
     * triggers a full sensor reset.  A 100 ms delay is required after this.
     */
    if (sensor_write(cam, OV5642_REG_RESET, OV5642_RESET_VAL) != 0) {
        return ARDUCAM_ERR_I2C;
    }
    cam->hal.delay_ms(100u);

    /* ---- 4. Verify OV5642 chip ID over I2C ----------------------------- */
    /*
     * OV5642 chip ID: high byte = 0x56, low byte = 0x42.
     * If these don't read back correctly, the I2C bus or sensor is faulty.
     */
    if ((sensor_read(cam, OV5642_CHIPID_HIGH, &chip_id_high) != 0) ||
        (sensor_read(cam, OV5642_CHIPID_LOW,  &chip_id_low)  != 0)) {
        return ARDUCAM_ERR_I2C;
    }
    if ((chip_id_high != 0x56u) || (chip_id_low != 0x42u)) {
        return ARDUCAM_ERR_I2C;
    }

    /* ---- 5. Load QVGA preview base init -------------------------------- */
    /*
     * This large table must be loaded first — it configures the full sensor
     * pipeline.  A 100 ms gap is required before loading the JPEG table.
     */
    if (sensor_load_table(cam, OV5642_QVGA_Preview) != 0) {
        return ARDUCAM_ERR_I2C;
    }
    cam->hal.delay_ms(100u);

    /* ---- 6. Load JPEG capture base table ------------------------------- */
    /*
     * Switches the output pipeline to JPEG mode at full QSXGA resolution.
     * The resolution scaling table applied next will crop/scale to the
     * desired output size.
     */
    if (sensor_load_table(cam, OV5642_JPEG_Capture_QSXGA) != 0) {
        return ARDUCAM_ERR_I2C;
    }
    cam->hal.delay_ms(100u);

    /* ---- 7. Apply resolution-specific output window -------------------- */
    if ((uint32_t)res >= RES_TABLE_LEN) {
        res = ARDUCAM_RES_JPEG_320x240;   /* safe fallback */
    }
    if (sensor_load_table(cam, RES_TABLE[(uint32_t)res]) != 0) {
        return ARDUCAM_ERR_I2C;
    }

    /* ---- 8. Mini-5MP-Plus specific tweaks ------------------------------ */
    /*
     * These three writes fine-tune the output for the Plus board revision.
     * 0x3818 / 0x3621 control mirror/flip and binning.
     * 0x4407 sets JPEG compression quality (0x08 = good quality for Plus).
     */
    if ((sensor_write(cam, 0x3818u, 0xa8u) != 0) ||
        (sensor_write(cam, 0x3621u, 0x10u) != 0) ||
        (sensor_write(cam, 0x3801u, 0xb0u) != 0) ||
        (sensor_write(cam, 0x4407u, 0x08u) != 0) ||
        (sensor_write(cam, 0x5888u, 0x00u) != 0) ||
        (sensor_write(cam, 0x5000u, 0xFFu) != 0)) {
        return ARDUCAM_ERR_I2C;
    }

    return ARDUCAM_OK;
}

arducam_err_t arducam_capture(arducam_t *cam, uint32_t timeout_ms)
{
    uint32_t elapsed = 0u;

    /* Clear FIFO: reset pointers and write-done flag */
    ac_write_reg(cam, AC_REG_FIFO_CTRL,
                 AC_FIFO_CLEAR_WR_DONE | AC_FIFO_CLEAR_RPTR | AC_FIFO_CLEAR_WPTR);

    /* Start single-frame capture */
    ac_write_reg(cam, AC_REG_FIFO_CTRL, AC_FIFO_START_CAPTURE);

    /* Poll until capture-done flag is set or timeout */
    while ((ac_read_reg(cam, AC_REG_TRIG) & AC_TRIG_CAP_DONE) == 0u) {
        cam->hal.delay_ms(1u);
        elapsed += 1u;
        if (elapsed >= timeout_ms) {
            return ARDUCAM_ERR_TIMEOUT;
        }
    }

    return ARDUCAM_OK;
}

uint32_t arducam_fifo_length(arducam_t *cam)
{
    uint32_t len;
    len  =  (uint32_t)ac_read_reg(cam, AC_REG_FIFO_SZ0);
    len |= ((uint32_t)ac_read_reg(cam, AC_REG_FIFO_SZ1)) <<  8u;
    len |= ((uint32_t)ac_read_reg(cam, AC_REG_FIFO_SZ2)) << 16u;
    return len;
}

uint8_t arducam_fifo_read_byte(arducam_t *cam)
{
    uint8_t tx = AC_CMD_SINGLE_READ;
    uint8_t rx = 0u;
    /* TX phase: send 0x3D command.
     * RX phase: ArduChip clocks out one FIFO byte. */
    cam->hal.spi_transact(cam->hal.ctx, &tx, 1u, &rx, 1u);
    return rx;
}

size_t arducam_fifo_read_burst(arducam_t *cam, uint8_t *buf, size_t len)
{
    uint8_t tx = AC_CMD_BURST_READ;
    /* TX phase: send 0x3C command once.
     * RX phase: ArduChip streams len bytes directly into buf.
     * CS must be held low for the entire call by spi_transact. */
    cam->hal.spi_transact(cam->hal.ctx, &tx, 1u, buf, (uint32_t)len);
    return len;
}