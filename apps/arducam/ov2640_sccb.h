#ifndef OV2640_SCCB_H
#define OV2640_SCCB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t (*ov2640_read_reg_fn)(void *ctx, uint8_t reg, uint8_t *val);
typedef int8_t (*ov2640_write_reg_fn)(void *ctx, uint8_t reg, uint8_t val);

typedef struct {
    ov2640_read_reg_fn read_reg;
    ov2640_write_reg_fn write_reg;
    void *ctx;
} ov2640_bus_t;

typedef struct {
    ov2640_bus_t bus;
} ov2640_t;

typedef struct {
    uint8_t midh;
    uint8_t midl;
    uint8_t pidh;
    uint8_t pidl;
} ov2640_id_t;

typedef enum {
    OV2640_OK = 0,
    OV2640_ERR_ARG = -1,
    OV2640_ERR_IO = -2,
    OV2640_ERR_ID = -3
} ov2640_err_t;

int8_t ov2640_init(ov2640_t *dev, const ov2640_bus_t *bus);
int8_t ov2640_read_reg(const ov2640_t *dev, uint8_t reg, uint8_t *val);
int8_t ov2640_write_reg(const ov2640_t *dev, uint8_t reg, uint8_t val);
int8_t ov2640_read_id(const ov2640_t *dev, ov2640_id_t *id);
int8_t ov2640_check_id(const ov2640_t *dev, ov2640_id_t *id);

#ifdef __cplusplus
}
#endif

#endif
