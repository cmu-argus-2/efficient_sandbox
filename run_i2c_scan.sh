export EFFCC_DIR="/home/argus/effcc/"
mkdir build
cd build
cmake -G Ninja .. -DEFF_STDIO_PORT=3
ninja
sudo /home/argus/effcc/bin/eff-flash apps/i2c_scan/scalar/i2c_scan.hex sram

# Note that the port may be different sometimes. Do
# ls /dev/ttyACM*
# to check which one is correct.
minicom -b 115200 -D /dev/ttyACM2