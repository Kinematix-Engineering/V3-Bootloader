#!/usr/bin/env sh
set -eu
cc -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-int-to-pointer-cast \
  -Itests/include -Ilib/kmxcommon -Ilib/kmxcpp_v3_bootloader -Ilib/udslib -Ilib/kmxcpp_uart -Isrc/uds \
  tests/programming_metadata.c src/uds/x22_ReadDataByIdentifier.c \
  src/uds/x34_RequestDownload.c src/uds/x36_TransferData.c src/flash/range.c \
  lib/kmxcpp_uart/protocol/protocol.c lib/kmxcpp_uart/protocol/checksum.c lib/kmxcpp_uart/cobs/cobs.c \
  lib/kmxcpp_v3_bootloader/v3_boot_table.c -o /tmp/kmx-programming-metadata-test
/tmp/kmx-programming-metadata-test
