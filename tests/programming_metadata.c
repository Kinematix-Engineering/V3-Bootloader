#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "flash_sector_table.h"
#include "KMX_UDS.h"
#include "handlers.h"
#include "boot_state.h"
#include "protocol/protocol.h"
#include "services/iso14229-1/22_readDataByIdentifer.h"
#include "services/iso14229-1/34_requestDownload.h"
#include "services/iso14229-1/36_transferData.h"

BootState boot_state;
static size_t written;
uint32_t boot_be32(const uint8_t data[4]) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | data[3];
}
bool flash_write_app(uint32_t address, const uint8_t *data, size_t length) {
    assert(address == APP_START && data[0] == 0x5a);
    written += length;
    return true;
}

uint64_t platform_uptime_ms(void) { return 0; }
void platform_serial(uint8_t serial[16]) { memset(serial, 0, 16); }

static void check(uint16_t pid, const uint8_t *expected, size_t length) {
    uint8_t buffer[32];
    UDS_22_RDBI_Request request = {.data_identifier = pid};
    UDS_22_RDBI_Response response = {.value_buf = buffer, .value_len = sizeof(buffer)};
    assert(x22_ReadDataByIdentifier(NULL, &request, &response, NULL) == UDS_NRC_PR);
    assert(response.value_len == length && !memcmp(buffer, expected, length));
    memset(buffer, 0xa5, sizeof(buffer));
    response.value_len = length - 1;
    assert(x22_ReadDataByIdentifier(NULL, &request, &response, NULL) == UDS_NRC_RESPONSE_TOO_LONG);
    assert(buffer[0] == 0xa5);
    response.value_buf = NULL;
    response.value_len = sizeof(buffer);
    assert(x22_ReadDataByIdentifier(NULL, &request, &response, NULL) == UDS_NRC_RESPONSE_TOO_LONG);
}

int main(void) {
    const flash_sector_table_entry_t entry = {0x12345678, 0x90abcdef, 0x10203040, 0x50607080};
    const uint8_t wire[] = {0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef,
                            0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80};
    uint8_t buffer[17] = {0};
    flash_sector_table_entry_t decoded = {0};
    assert(flash_sector_table_entry_encode(&entry, buffer, sizeof(buffer)));
    assert(!memcmp(buffer, wire, sizeof(wire)) && buffer[16] == 0);
    assert(flash_sector_table_entry_decode(wire, sizeof(wire), &decoded));
    assert(decoded.address == entry.address && decoded.sector_size == entry.sector_size);
    assert(decoded.count == entry.count && decoded.flags == entry.flags);
    assert(!flash_sector_table_entry_encode(&entry, buffer, 15));
    assert(!flash_sector_table_entry_encode(NULL, buffer, 16));
    assert(!flash_sector_table_entry_encode(&entry, NULL, 16));
    assert(!flash_sector_table_entry_decode(wire, 15, &decoded));
    assert(decoded.address == entry.address);
    assert(!flash_sector_table_entry_decode(NULL, 16, &decoded));
    assert(!flash_sector_table_entry_decode(wire, 16, NULL));

    const uint8_t sectors[] = {0,0,0,0, 0,0,0x20,0, 0,0,0,128, 0,0,0,0};
    const uint8_t asw[] = {0,0,0x40,0, 0,7,0xc0,0};
    const uint8_t erased[] = {0xff};
    const uint8_t transfer_size[] = {0,0,8,0};
    check(KMX_PID_PROGRAMMING_SECTOR_TABLE, sectors, sizeof(sectors));
    check(KMX_PID_PROGRAMMING_ASW_RANGE, asw, sizeof(asw));
    check(KMX_PID_PROGRAMMING_ERASE_BYTE, erased, sizeof(erased));
    check(KMX_PID_PROGRAMMING_TRANSFER_BLOCK_SIZE, transfer_size, sizeof(transfer_size));

    boot_state.authenticated = true;
    memset(boot_state.erased, 0xff, sizeof(boot_state.erased));
    const uint8_t address[] = {0,0,0x40,0}, size[] = {0,0,0x20,0};
    UDS_34_RD_Request download = {0, address, 4, size, 4};
    UDS_34_RD_Response response = {0};
    assert(x34_RequestDownload(NULL, &download, &response, NULL) == UDS_NRC_PR);
    assert(response.maxNumberOfBlockLengthLen == 2);
    assert(response.maxNumberOfBlockLength[0] == 8 && response.maxNumberOfBlockLength[1] == 2);

    uint8_t payload[2066];
    memset(payload, 0x5a, sizeof(payload));
    payload[0] = 0x36;
    payload[1] = 1;
    uint8_t frame[KMX_PROTOCOL_MAX_FRAME], decoded_frame[KMX_PROTOCOL_MAX_DECODED];
    size_t frame_len = kmx_protocol_encode(KMX_MESSAGE_UDS_DIAGNOSTIC,
        payload, 2050, frame, sizeof(frame));
    assert(frame_len);
    kmx_message_t message;
    assert(kmx_protocol_decode(frame, frame_len - 1, decoded_frame,
        sizeof(decoded_frame), &message) == KMX_PROTOCOL_OK);
    assert(message.payload_len == 2050 && !memcmp(message.payload, payload, 2050));
    UDS_36_TD_Request transfer = {1, message.payload + 2, message.payload_len - 2};
    assert(x36_TransferData(NULL, &transfer, NULL, NULL) == UDS_NRC_PR);
    assert(written == 2048 && boot_state.transferred == 2048);
    transfer.blockSequenceCounter = 2;
    transfer.transferRequestParameterRecord = payload + 2;
    transfer.transferRequestParameterRecordLen = 2064;
    assert(x36_TransferData(NULL, &transfer, NULL, NULL) == UDS_NRC_IMLOIF);
    assert(written == 2048 && boot_state.transferred == 2048);
    puts("programming metadata passed");
}
