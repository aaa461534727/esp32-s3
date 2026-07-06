#ifndef RID_PARSE_GB_H
#define RID_PARSE_GB_H

#include <stdint.h>
#include <stdbool.h>

/* 前向声明 (rid_parse.h 里的 rid_info) */
struct rid_info;

/*
 * GB 46750-2025 TLV 编码函数
 * 将 rid_info 编码为 GB TLV 字节流
 * buf: 输出 buffer
 * buf_len: buffer 大小
 * info: 输入数据
 * 返回: TLV 字节数，0 = 错误
 */
int rid_gb_encode(unsigned char *buf, int buf_len, struct rid_info *info);

#endif
