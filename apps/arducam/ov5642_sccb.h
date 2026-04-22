#ifndef OV5642_SCCB_H
#define OV5642_SCCB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t (*ov5642_read_reg_fn)(void *ctx, uint16_t reg, uint8_t *val);
typedef int8_t (*ov5642_write_reg_fn)(void *ctx, uint16_t reg, uint8_t val);

typedef struct {
    ov5642_read_reg_fn read_reg;
    ov5642_write_reg_fn write_reg;
    void *ctx;
} ov5642_bus_t;

typedef struct {
    ov5642_bus_t bus;
} ov5642_t;

typedef struct {
    uint8_t chip_id_high;
    uint8_t chip_id_low;
} ov5642_id_t;

typedef enum {
    OV5642_OK = 0,
    OV5642_ERR_ARG = -1,
    OV5642_ERR_IO = -2,
    OV5642_ERR_ID = -3
} ov5642_err_t;

int8_t ov5642_init(ov5642_t *dev, const ov5642_bus_t *bus);
int8_t ov5642_read_reg(const ov5642_t *dev, uint16_t reg, uint8_t *val);
int8_t ov5642_write_reg(const ov5642_t *dev, uint16_t reg, uint8_t val);
int8_t ov5642_read_id(const ov5642_t *dev, ov5642_id_t *id);
int8_t ov5642_check_id(const ov5642_t *dev, ov5642_id_t *id);

#ifdef __cplusplus
}
#endif

#endif
