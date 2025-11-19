//
// Created by darren on 2025/11/19.
//

#ifndef ATLVC_ATLVC_CHECK_H
#define ATLVC_ATLVC_CHECK_H

#include "stdint.h"

// 逐位计算CRC8（多项式0x07，初始值0x00，异或输出0x00，无数据反转）
uint8_t atlvc_crc8_checksum(const uint8_t* data, size_t len);

uint8_t atlvc_xor_checksum(const uint8_t* data, size_t len);

#endif //ATLVC_ATLVC_CHECK_H