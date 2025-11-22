# v3bl

Modified by Nick Daria for Kinematix use. See original repo for details.

## Bootloader update
The bootloader will never write to its own flash area directly.
However, the user code can write there.
Thus, to update the bootloader, one can ship a user-space program,
that contains the new version of the bootloader and copies it to the
appropriate place in flash.

Such a program is generated during build in files `update-bootloader*.uf2`.
If you're already running UF2 bootloader, the easiest way to update
it, is to just copy this file to the exposed MSD drive.

The build also generates `update-bootloader*.ino` with an equivalent Arduino
sketch. You can copy&paste it into Arduino IDE and upload it to the device.

## Build

### Requirements

* `make` and a Unix environment (I used Debian WSL)
* `arm-none-eabi-gcc` (install `gcc-arm-none-eabi` via apt)
* `openocd` - you can use the one coming with Arduino (after your install the M0 board support)


### Build commands

Run:

```
make r
```

### Distribute

`scripts/make_kflash.py` is able to generate a file that can be flashed via KFlash using an Atmel SAM-ICE