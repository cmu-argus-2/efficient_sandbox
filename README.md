# effcc_workspace

Starter workspace for experimenting with the Efficient Computer E1x EVK using the effcc SDK.

This repository is meant to be a simple place to:
- build and flash small test programs
- learn the E1x SDK APIs
- prototype peripherals like GPIO, UART, and I2C
- keep lab bring-up code in one organized workspace

# How to run manually (quickstart as example)
```
export EFFCC_DIR="/home/argus/effcc/"
mkdir build
cd build
cmake -G Ninja .. -DEFF_STDIO_PORT=3
ninja
sudo /home/argus/effcc/bin/eff-flash quickstart/fabric/quickstart.hex sram

sudo minicom -D /dev/ttyACM2
```

# run.sh
chmod +x run.sh
./run.sh {appname}

# Notes

## EVK reminders
- Flashing to SRAM is temporary.
- STDIO is expected on the EVK serial interface.
- Check boot switch configuration before flashing.
- If scanning I2C devices on Arduino headers, SW10 may matter.

## Things to verify in lab
- Which I2C bus is actually wired
- Whether Arduino headers or GPIO headers are being used
- Whether pull-ups are present
- What devices are physically connected