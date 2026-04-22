set(ARDUCAM_ROOT "/Users/andrewkim/Downloads/Arduino-master/ArduCAM")
set(ARDUCAM_RPI "${ARDUCAM_ROOT}/examples/RaspberryPi")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ARDUCAM_ROOT}
    ${ARDUCAM_RPI}
)

add_compile_definitions(
    RASPBERRY_PI
    OV5642_MINI_5MP_PLUS
)

add_eff_app(arducam_driver_port
    TYPE exe
    ARCHS e1x
    TARGETS scalar
    SOURCE
        main.cpp
        arducam_arch_e1x.cpp
        ${ARDUCAM_ROOT}/ArduCAM.cpp
)
