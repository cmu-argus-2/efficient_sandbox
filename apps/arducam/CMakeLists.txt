add_eff_app(arducam
    TYPE exe
    ARCHS e1x
    TARGETS scalar
    SOURCE main.c arducam_spi.c ov2640_sccb.c
)
