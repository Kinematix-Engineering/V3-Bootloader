cmake_minimum_required(VERSION 3.20)
project(v3_bootloader C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/udslib/udslib.h)
    message(FATAL_ERROR "Initialize udslib with: git submodule update --init")
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/kmxcpp_v3_bootloader/v3_boot_table.h)
    message(FATAL_ERROR "Initialize kmxcpp_v3_bootloader with: git submodule update --init")
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/kmxcommon/KMX_UDS.h)
    message(FATAL_ERROR "Initialize kmxcommon with: git submodule update --init")
endif()

set(UDSLIB_SOURCES
    lib/udslib/services/iso14229-1/10_diagnosticSessionControl.c
    lib/udslib/lookup/uds_buf.c
    lib/udslib/session_server/uds_server.c
    lib/udslib/services/iso14229-1/11_ecuReset.c
    lib/udslib/services/iso14229-1/22_readDataByIdentifer.c
    lib/udslib/services/iso14229-1/27_securityAccess.c
    lib/udslib/services/iso14229-1/31_routineControl.c
    lib/udslib/services/iso14229-1/34_requestDownload.c
    lib/udslib/services/iso14229-1/36_transferData.c
    lib/udslib/services/iso14229-1/37_requestTransferExit.c
    lib/udslib/services/iso14229-1/download_upload_codec.c
)

add_executable(bootloader-kmx_v3.elf
    src/startup.c
    src/platform.c
    src/main.c
    src/flash/table_data.c
    src/flash/checksum.c
    src/flash/range.c
    src/flash/nvm.c
    src/usb/device.c
    lib/kmxcpp_uart/cobs/cobs.c
    lib/kmxcpp_uart/protocol/protocol.c
    lib/kmxcpp_uart/protocol/checksum.c
    src/usb/transport.c
    src/crypto/aes128.c
    src/uds/boot_services.c
    src/uds/x10_DiagnosticSessionControl.c
    src/uds/x11_EcuReset.c
    src/uds/x22_ReadDataByIdentifier.c
    src/uds/x27_SecurityAccess.c
    src/uds/x31_RoutineControl.c
    src/uds/x34_RequestDownload.c
    src/uds/x36_TransferData.c
    src/uds/x37_RequestTransferExit.c
    lib/kmxcpp_v3_bootloader/v3_boot_table.c
    ${UDSLIB_SOURCES}
)

target_include_directories(bootloader-kmx_v3.elf PRIVATE
    src/vendor/cmsis/CMSIS/Include
    src/vendor/samd51/include
    lib/udslib
    lib/kmxcommon
    lib/kmxcpp_uart
    lib/kmxcpp_uart/cobs
    lib/kmxcpp_v3_bootloader)
target_compile_definitions(bootloader-kmx_v3.elf PRIVATE __SAMD51P20A__)
target_compile_options(bootloader-kmx_v3.elf PRIVATE
    -mcpu=cortex-m4 -mthumb -Os -g -ffunction-sections -fdata-sections
    -fno-strict-aliasing -Wall -Wextra -Werror -Wno-unused-parameter)
target_link_options(bootloader-kmx_v3.elf PRIVATE
    -mcpu=cortex-m4 -mthumb -Os -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,--gc-sections -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/bootloader-kmx_v3.map
    -T${CMAKE_CURRENT_SOURCE_DIR}/scripts/samd51.ld)

add_custom_command(TARGET bootloader-kmx_v3.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary --gap-fill 0xff --pad-to 0x4000
            $<TARGET_FILE:bootloader-kmx_v3.elf>
            ${CMAKE_CURRENT_BINARY_DIR}/bootloader-kmx_v3.bin
    COMMAND arm-none-eabi-size $<TARGET_FILE:bootloader-kmx_v3.elf>
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/bootloader-kmx_v3.bin)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/boot_image.c
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/bin_to_c.py
            ${CMAKE_CURRENT_BINARY_DIR}/bootloader-kmx_v3.bin
            ${CMAKE_CURRENT_BINARY_DIR}/boot_image.c
    DEPENDS bootloader-kmx_v3.elf ${CMAKE_CURRENT_SOURCE_DIR}/scripts/bin_to_c.py)

add_executable(bootupdate-kmx_v3.elf
    src/startup.c
    src/update/main.c
    src/update/table_preserve.c
    lib/kmxcpp_v3_bootloader/v3_boot_table.c
    ${CMAKE_CURRENT_BINARY_DIR}/boot_image.c)
target_include_directories(bootupdate-kmx_v3.elf PRIVATE
    src/vendor/cmsis/CMSIS/Include
    src/vendor/samd51/include
    lib/kmxcpp_v3_bootloader)
target_compile_definitions(bootupdate-kmx_v3.elf PRIVATE __SAMD51P20A__)
target_compile_options(bootupdate-kmx_v3.elf PRIVATE
    -mcpu=cortex-m4 -mthumb -Os -g -ffunction-sections -fdata-sections
    -Wall -Wextra -Werror -Wno-unused-parameter)
target_link_options(bootupdate-kmx_v3.elf PRIVATE
    -mcpu=cortex-m4 -mthumb -Os -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,--gc-sections -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/bootupdate-kmx_v3.map
    -T${CMAKE_CURRENT_SOURCE_DIR}/scripts/samd51_update.ld)
add_custom_command(TARGET bootupdate-kmx_v3.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:bootupdate-kmx_v3.elf>
            ${CMAKE_CURRENT_BINARY_DIR}/bootupdate-kmx_v3.bin
    COMMAND arm-none-eabi-size $<TARGET_FILE:bootupdate-kmx_v3.elf>
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/bootupdate-kmx_v3.bin)

# KFlash packaging is deferred until kdc is available in the build environment.
# add_custom_target(kflash ...)
