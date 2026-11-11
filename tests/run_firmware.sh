#!/usr/bin/env sh
set -eu
cc -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -Itests/include -Ilib/kmxcpp_v3_bootloader -Ilib/udslib -Isrc/uds -Isrc/flash -Isrc/usb -Isrc/crypto \
  tests/firmware_protocol.c src/flash/range.c src/uds/boot_services.c src/uds/x11_EcuReset.c \
  src/uds/x22_ReadDataByIdentifier.c src/uds/x27_SecurityAccess.c \
  src/uds/x31_RoutineControl.c src/uds/x34_RequestDownload.c \
  src/uds/x36_TransferData.c src/uds/x37_RequestTransferExit.c \
  src/crypto/aes128.c src/usb/cobs.c \
  lib/udslib/lookup/uds_buf.c lib/udslib/session_server/uds_server.c \
  lib/udslib/services/iso14229-1/11_ecuReset.c \
  lib/udslib/services/iso14229-1/22_readDataByIdentifer.c \
  lib/udslib/services/iso14229-1/27_securityAccess.c \
  lib/udslib/services/iso14229-1/31_routineControl.c \
  lib/udslib/services/iso14229-1/34_requestDownload.c \
  lib/udslib/services/iso14229-1/36_transferData.c \
  lib/udslib/services/iso14229-1/37_requestTransferExit.c \
  lib/udslib/services/iso14229-1/download_upload_codec.c \
  -o /tmp/kmx-firmware-protocol-test
/tmp/kmx-firmware-protocol-test
cc -std=c11 -Wall -Wextra -Werror -Isrc/flash \
  tests/flash_checksum.c src/flash/checksum.c \
  -o /tmp/kmx-flash-checksum-test
/tmp/kmx-flash-checksum-test
cc -std=c11 -Wall -Wextra -Werror -Isrc/usb -Isrc/uds \
  tests/boot_handoff.c src/usb/transport.c src/usb/cobs.c \
  -o /tmp/kmx-boot-handoff-test
/tmp/kmx-boot-handoff-test
cc -std=c11 -Wall -Wextra -Werror -Ilib/kmxcpp_v3_bootloader -Isrc/update \
  tests/v3_boot_table.c lib/kmxcpp_v3_bootloader/v3_boot_table.c src/flash/table_data.c src/update/table_preserve.c \
  -o /tmp/kmx-boot-table-test
/tmp/kmx-boot-table-test
cc -std=c11 -Wall -Wextra -Werror \
  lib/kmxcpp_v3_bootloader/test/v3_boot_table_validation_test.c \
  lib/kmxcpp_v3_bootloader/v3_boot_table.c \
  -o /tmp/kmx-shared-table-test
/tmp/kmx-shared-table-test
cc -std=c11 -Wall -Wextra -Werror -Ilib/kmxcpp_v3_bootloader \
  -c lib/kmxcpp_v3_bootloader/v3_boot_table.c -o /tmp/kmx-v3-boot-table.o
c++ -std=c++17 -Wall -Wextra -Werror -Ilib/kmxcpp_v3_bootloader \
  tests/v3_boot_table_cpp.cpp /tmp/kmx-v3-boot-table.o \
  -o /tmp/kmx-boot-table-cpp-test
/tmp/kmx-boot-table-cpp-test
c++ -std=c++17 -Wall -Wextra -Werror -x c++ -Ilib/kmxcpp_v3_bootloader \
  tests/v3_boot_table_cpp.cpp lib/kmxcpp_v3_bootloader/v3_boot_table.c \
  -o /tmp/kmx-boot-table-native-cpp-test
/tmp/kmx-boot-table-native-cpp-test
