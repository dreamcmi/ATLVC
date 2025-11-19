//
// Created by darren on 2025/11/19.
//

#ifndef ATLVC_ATLVC_ERR_H
#define ATLVC_ATLVC_ERR_H
#include <stdint.h>

typedef int32_t atlvc_err_t;

#define ATLVC_ERR_SUCCESS        0   // 成功
#define ATLVC_ERR_FAILURE        1   // 通用失败
#define ATLVC_ERR_INPUT_NULL     2   // 输入为空指针
#define ATLVC_ERR_INVALID_FRAME  3   // 帧格式无效（长度不足）
#define ATLVC_ERR_CMD_NOT_FOUND  4   // 未找到匹配的指令规则
#define ATLVC_ERR_DATA_LEN_ERR   5   // 数据长度超出范围
#define ATLVC_ERR_CHECKSUM_ERR   6   // 校验失败
#define ATLVC_ERR_HANDLER_NULL   7   // 回调函数为空

#endif //ATLVC_ATLVC_ERR_H