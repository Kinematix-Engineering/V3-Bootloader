#!/usr/bin/env sh
set -eu

# Requires an admin-enabled kdiag CLI and the KDiag master permit.
# Existing hardware/package parameters:
#   name: V3 Bootloader
#   version: v3bl-1.2.0
#   modulePart: KEL-CTRL-V3*
#   moduleType: 0x10
#   permit: KDiag
#   toolchain: Atmel atprogram
#   device: ATSAMD51P20A
#
kdiag kflash create --output build/bootloader-kmx_v3.kflash --permit KDiag \
  --name 'V3 Bootloader' --version v3bl-1.2.0 \
  --modulePart 'KEL-CTRL-V3*' --moduleType 0x10 \
  --toolchain 'Atmel atprogram' --flag 'device|ATSAMD51P20A' \
  --segment 'bootloader|0x0|build/bootloader-kmx_v3.bin' \
  --mapping 'v3bl_factorydata|0x3EF0'

kdiag kflash create --output build/bootupdate-kmx_v3.kflash --permit KDiag \
  --name 'V3 Bootloader Update' --version v3bl-1.2.0 \
  --modulePart 'KEL-CTRL-V3*' --moduleType 0x10 \
  --toolchain 'Atmel atprogram' --flag 'device|ATSAMD51P20A' \
  --segment 'application|0x4000|build/bootupdate-kmx_v3.bin' \
  --mapping 'v3bl_factorydata|0x3EF0'
