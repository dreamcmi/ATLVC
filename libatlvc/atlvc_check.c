//
// Created by darren on 2025/11/19.
//

#include "atlvc_check.h"


// 逐位计算CRC8（多项式0x07，初始值0x00，异或输出0x00，无数据反转）
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


uint8_t atlvc_xor_checksum(const uint8_t* data, size_t len) {
    uint8_t xor = 0x00;
    if (data == NULL || len == 0) {
        return xor;  // 输入无效时返回0（避免异常）
    }
    for (size_t i = 0; i < len; i++) {
        xor ^= data[i];
    }
    return xor;
}