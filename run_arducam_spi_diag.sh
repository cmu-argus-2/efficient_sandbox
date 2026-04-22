#!/bin/bash

set -e

export EFFCC_DIR="${EFFCC_DIR:-/home/argus/effcc/}"

mkdir -p build
cd build
cmake -G Ninja .. -DEFF_STDIO_PORT=3
ninja arducam_spi_diag_scalar
sudo "${EFFCC_DIR}/bin/eff-flash" apps/arducam_spi_diag/scalar/arducam_spi_diag.hex sram
