set(ARDUCAM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor")
set(ARDUCAM_RPI "${ARDUCAM_ROOT}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ARDUCAM_ROOT}
    ${ARDUCAM_RPI}
)

add_compile_definitions(
    RASPBERRY_PI
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
