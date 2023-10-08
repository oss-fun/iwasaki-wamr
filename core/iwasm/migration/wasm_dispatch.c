#include <stdio.h>
#include <stdlib.h>

#include "../interpreter/wasm_runtime.h"
#include "wasm_migration.h"
#include "wasm_dump.h"

static uint64
read_leb(const uint8 *buf, uint32 *p_offset, uint32 maxbits, bool sign)
{
    uint64 result = 0, byte;
    uint32 offset = *p_offset;
    uint32 shift = 0;

    while (true) {
        byte = buf[offset++];
        result |= ((byte & 0x7f) << shift);
        shift += 7;
        if ((byte & 0x80) == 0) {
            break;
        }
    }
    if (sign && (shift < maxbits) && (byte & 0x40)) {
        /* Sign extend */
        result |= (~((uint64)0)) << shift;
    }
    *p_offset = offset;
    return result;
}

#define skip_leb(p) while (*p++ & 0x80)

#define read_leb_int64(p, p_end, res)              \
    do {                                           \
        uint8 _val = *p;                           \
        if (!(_val & 0x80)) {                      \
            res = (int64)_val;                     \
            if (_val & 0x40)                       \
                /* sign extend */                  \
                res |= 0xFFFFFFFFFFFFFF80LL;       \
            p++;                                   \
            break;                                 \
        }                                          \
        uint32 _off = 0;                           \
        res = (int64)read_leb(p, &_off, 64, true); \
        p += _off;                                 \
    } while (0)

#define read_leb_uint32(p, p_end, res)               \
    do {                                             \
        uint8 _val = *p;                             \
        if (!(_val & 0x80)) {                        \
            res = _val;                              \
            p++;                                     \
            break;                                   \
        }                                            \
        uint32 _off = 0;                             \
        res = (uint32)read_leb(p, &_off, 32, false); \
        p += _off;                                   \
    } while (0)

#define read_leb_int32(p, p_end, res)              \
    do {                                           \
        uint8 _val = *p;                           \
        if (!(_val & 0x80)) {                      \
            res = (int32)_val;                     \
            if (_val & 0x40)                       \
                /* sign extend */                  \
                res |= 0xFFFFFF80;                 \
            p++;                                   \
            break;                                 \
        }                                          \
        uint32 _off = 0;                           \
        res = (int32)read_leb(p, &_off, 32, true); \
        p += _off;                                 \
    } while (0)

#define HANDLE_OP(opcode) HANDLE_##opcode:

int dispatch(uint8 *ip) {
    goto *handle_table[*frame_ip];
    HANDLE_OP(WASM_OP_UNREACHABLE)
    {
        
    }
}