#include <stdio.h>

uint8_t atlvc_crc8_checksum(const uint8_t* data, size_t len) {
    uint8_t crc = 0x00;
    if (data == NULL || len == 0) {
        return crc;
    }
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc ^ 0x00;
}

int main(void) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x01, 0x02, 0x03};
    uint8_t crc = atlvc_crc8_checksum(data, sizeof(data));
    printf("CRC8 of {0x01, 0x02, 0x03, 0x01, 0x02, 0x03} = 0x%02X\n", crc);
    return 0;
}