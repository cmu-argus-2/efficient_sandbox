add_eff_app(ov2640_i2c_test
    TYPE exe
    ARCHS e1x
    TARGETS scalar
    SOURCE main.c ../arducam/ov2640_sccb.c
)
