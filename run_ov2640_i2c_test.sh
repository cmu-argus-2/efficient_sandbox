#!/bin/bash

set -e

export EFFCC_DIR="${EFFCC_DIR:-/home/argus/effcc/}"

mkdir -p build
cd build
cmake -G Ninja .. -DEFF_STDIO_PORT=3
ninja ov2640_i2c_test_scalar
sudo "${EFFCC_DIR}/bin/eff-flash" apps/ov2640_i2c_test/scalar/ov2640_i2c_test.hex sram

# Note that the port may be different sometimes. Do
# ls /dev/ttyACM*
# to check which one is correct.
minicom -b 115200 -D /dev/ttyACM2
