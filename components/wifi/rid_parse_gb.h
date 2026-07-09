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

/*
 * GB 46750-2025 TLV 解析函数
 * 将 GB TLV 字节流解析到 rid_info
 * data: 输入 buffer（从 0xFF 开始）
 * offset: 0xFF 的偏移
 * packet_len: 从 offset 到 buffer 末尾的长度
 * info: 输出
 * 返回: 解析后的 idx，<=0 = 失败
 */
int rid_parse_gb(unsigned char *data, int offset, int packet_len, struct rid_info *info);

#endif
