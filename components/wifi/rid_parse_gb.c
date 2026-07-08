#include "rid_parse_gb.h"
#include "rid_parse.h"
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

/* Little-endian write helpers (对应 rid_parse_gb.c 的 LE16_TO_HOST/LE32_TO_HOST) */
static inline void w16_le(uint8_t *buf, uint16_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
}
static inline void w32_le(uint8_t *buf, uint32_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}
static inline void w48_le(uint8_t *buf, uint64_t val) {
    for (int i = 0; i < 6; i++)
        buf[i] = (val >> (8 * i)) & 0xFF;
}

/*
 * rid_gb_encode — 严格按照 GB 46750-2025 TLV 格式编码
 * 对应 C738VR rid_parse_gb.c 的解析逻辑逆运算
 */
int rid_gb_encode(unsigned char *buf, int buf_len, struct rid_info *info)
{
    if (!buf || !info || buf_len < 128) return 0;
    int idx = 0;

    /* 数据类型 = 0xFF */
    buf[idx++] = 0xFF;
    /* 版本号: 高3bit固定001, 低5bit版本号1 → 001_00001 = 0x21 */
    buf[idx++] = 0x21;
    /* 数据长度占位 */
    int len_pos = idx;
    idx++;

    /* === 构造 bitmap (3字节，与 rid_parse_gb.c 完全对应) === */
    uint8_t bmp[3] = {0};

    /* Byte 0: fields 001..007, bit0 = 扩展位 (1=还有下一字节) */
    if (info->upi[0] || info->sn[0])  bmp[0] |= 0x80; /* 001 UPI */
    if (info->reg_id[0])              bmp[0] |= 0x40; /* 002 reg_id */
    /* 003 category — 总是有值 */
    bmp[0] |= 0x20;
    /* 004 class — 总是有值 */
    bmp[0] |= 0x10;
    /* 005 ilot_loc_type — 总是有值 */
    bmp[0] |= 0x08;
    /* 006 遥控站位置 — 总是有值 */
    bmp[0] |= 0x04;
    /* 007 遥控站高度 — 总是有值 */
    bmp[0] |= 0x02;
    bmp[0] |= 0x01; /* bit0=1 → 有 Byte 1 */

    /* Byte 1: fields 008..014 */
    bmp[1] |= 0x80; /* 008 UA 位置 — 总是有值 */
    bmp[1] |= 0x40; /* 009 航迹角 */
    bmp[1] |= 0x20; /* 010 地速 */
    bmp[1] |= 0x10; /* 011 相对高度 */
    bmp[1] |= 0x08; /* 012 垂直速度 */
    bmp[1] |= 0x04; /* 013 大地高度 */
    bmp[1] |= 0x02; /* 014 气压高度 */
    bmp[1] |= 0x01; /* bit0=1 → 有 Byte 2 */

    /* Byte 2: fields 015..021 */
    bmp[2] |= 0x80; /* 015 运行状态 */
    bmp[2] |= 0x40; /* 016 坐标系类型 */
    bmp[2] |= 0x20; /* 017 水平精度 */
    bmp[2] |= 0x10; /* 018 垂直精度 */
    bmp[2] |= 0x08; /* 019 速度精度 */
    bmp[2] |= 0x04; /* 020 时间戳 */
    bmp[2] |= 0x02; /* 021 时间戳精度 */
    bmp[2] &= ~0x01; /* bit0=0 → 结束 */

    buf[idx++] = bmp[0];
    buf[idx++] = bmp[1];
    buf[idx++] = bmp[2];

    /* === 内容区 (按 bitmap 顺序编码) === */

    /* Field 001: UPI (20 B ASCII, big-endian = 直接拷贝) */
    if (bmp[0] & 0x80) {
        const char *s = info->upi[0] ? info->upi :
                        info->sn[0] ? info->sn : "123456BLE501236AHT33";
        int n = strlen(s);
        for (int i = 0; i < 20; i++)
            buf[idx + i] = (i < n) ? s[i] : ' ';
        idx += 20;
    }

    /* Field 002: reg_id (8 B ASCII) */
    if (bmp[0] & 0x40) {
        int n = strlen(info->reg_id);
        for (int i = 0; i < 8; i++)
            buf[idx + i] = (i < n) ? info->reg_id[i] : 0;
        idx += 8;
    }

    /* Field 003: gb_category (1 B) */
    if (bmp[0] & 0x20) {
        buf[idx++] = (uint8_t)(info->gb_category);
    }

    /* Field 004: gb_class (1 B) */
    if (bmp[0] & 0x10) {
        buf[idx++] = (uint8_t)(info->gb_class);
    }

    /* Field 005: ilot_loc_type (1 B) */
    if (bmp[0] & 0x08) {
        buf[idx++] = (uint8_t)(info->ilot_loc_type);
    }

    /* Field 006: 遥控站位置 (8 B, LE int32 × 1e-7) */
    if (bmp[0] & 0x04) {
        float ilon = info->ilot_lon_gb ? info->ilot_lon_gb :
                     info->ilotLon ? info->ilotLon : 0.0f;
        float ilat = info->ilot_lat_gb ? info->ilot_lat_gb :
                     info->ilotLat ? info->ilotLat : 0.0f;
        w32_le(buf + idx, (uint32_t)((int32_t)(ilon * 1e7)));
        w32_le(buf + idx + 4, (uint32_t)((int32_t)(ilat * 1e7)));
        idx += 8;
    }

    /* Field 007: 遥控站高度 (2 B LE u16, (real+1000)*2) */
    if (bmp[0] & 0x02) {
        float h = info->ilot_height ? info->ilot_height : 10.0f;
        int16_t raw = (int16_t)((h + 1000.0f) * 2 + 0.5f);
        w16_le(buf + idx, (uint16_t)raw);
        idx += 2;
    }

    /* Field 008: UA 位置 (8 B, LE int32 × 1e-7) */
    if (bmp[1] & 0x80) {
        float lon = info->lon;
        float lat = info->lat;
        w32_le(buf + idx, (uint32_t)((int32_t)(lon * 1e7)));
        w32_le(buf + idx + 4, (uint32_t)((int32_t)(lat * 1e7)));
        idx += 8;
    }

    /* Field 009: 航迹角 (2 B LE u16, real*10, 0xFFFF=unknown) */
    if (bmp[1] & 0x40) {
        float d = info->direction;
        uint16_t raw = (d < 0 || d > 360) ? 0xFFFF : (uint16_t)(d * 10 + 0.5f);
        w16_le(buf + idx, raw);
        idx += 2;
    }

    /* Field 010: 地速 (2 B LE u16, real*10, 0xFFFF=unknown) */
    if (bmp[1] & 0x20) {
        float s = info->speed;
        uint16_t raw = (s < 0) ? 0xFFFF : (uint16_t)(s * 10 + 0.5f);
        w16_le(buf + idx, raw);
        idx += 2;
    }

    /* Field 011: 相对高度 (2 B LE i16, (real+9000)*2) */
    if (bmp[1] & 0x10) {
        float alt = info->alt;
        int16_t raw = (int16_t)((alt + 9000.0f) * 2 + 0.5f);
        w16_le(buf + idx, (uint16_t)raw);
        idx += 2;
    }

    /* Field 012: 垂直速度 (1 B: bit7=sign, low7=abs*2, 0xFF=unknown) */
    if (bmp[1] & 0x08) {
        float vs = info->v_speed;
        uint8_t raw;
        if (vs < -31 || vs > 31) raw = 0xFF; /* unknown */
        else if (vs < 0) raw = 0x80 | (uint8_t)((-vs) * 2 + 0.5f);
        else raw = (uint8_t)(vs * 2 + 0.5f);
        buf[idx++] = raw;
    }

    /* Field 013: 大地高度 (2 B LE i16, (real+1000)*2) */
    if (bmp[1] & 0x04) {
        float h = info->geo_high;
        int16_t raw = (int16_t)((h + 1000.0f) * 2 + 0.5f);
        w16_le(buf + idx, (uint16_t)raw);
        idx += 2;
    }

    /* Field 014: 气压高度 (2 B LE i16, (real+1000)*2) */
    if (bmp[1] & 0x02) {
        float h = info->air_high;
        int16_t raw = (int16_t)((h + 1000.0f) * 2 + 0.5f);
        w16_le(buf + idx, (uint16_t)raw);
        idx += 2;
    }

    /* Field 015: 运行状态 (1 B enum) */
    if (bmp[2] & 0x80) {
        buf[idx++] = (uint8_t)(info->status > 0 ? info->status : 0);
    }

    /* Field 016: 坐标系类型 (1 B: 0=WGS-84) */
    if (bmp[2] & 0x40) {
        buf[idx++] = (uint8_t)(info->coord_system ? info->coord_system : 0);
    }

    /* Field 017: 水平精度 (1 B) */
    if (bmp[2] & 0x20) {
        buf[idx++] = (uint8_t)(info->hor_accuracy ? info->hor_accuracy : 2);
    }

    /* Field 018: 垂直精度 (1 B) */
    if (bmp[2] & 0x10) {
        buf[idx++] = (uint8_t)(info->ver_accuracy ? info->ver_accuracy : 2);
    }

    /* Field 019: 速度精度 (1 B) */
    if (bmp[2] & 0x08) {
        buf[idx++] = (uint8_t)(info->speed_accuracy ? info->speed_accuracy : 2);
    }

    /* Field 020: 时间戳 (6 B LE u48, Unix epoch ms) */
    if (bmp[2] & 0x04) {
        uint64_t ms = info->unix_ts_ms;
        if (ms == 0) {
            struct timeval tv; gettimeofday(&tv, NULL);
            ms = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
        }
        w48_le(buf + idx, ms);
        idx += 6;
    }

    /* Field 021: 时间戳精度 (1 B) */
    if (bmp[2] & 0x02) {
        buf[idx++] = (uint8_t)(info->ts_accuracy ? info->ts_accuracy : 0);
    }

    /* 回填数据长度 (内容区长度, 不含 header 和 bitmap) */
    int content_len = idx - len_pos - 1 - 3; /* total - header(3B) - bitmap(3B) */
    buf[len_pos] = (uint8_t)content_len;

    return idx;
}
