cmake_minimum_required(VERSION 3.15)

project(effcc_workspace)

# Adjust this path if effcc is installed somewhere else
file(REAL_PATH "~/effcc/sdk" EFF_SDK_ROOT_DIR)
include(${EFF_SDK_ROOT_DIR}/setup_sdk.cmake)
add_subdirectory(${EFF_SDK_ROOT_DIR} sdk)

add_subdirectory(apps/spi_test)
add_subdirectory(apps/arducam)
add_subdirectory(apps/arducam_spi_test)
add_subdirectory(apps/ov2640_i2c_test)
