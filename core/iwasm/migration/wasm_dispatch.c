#include <stdio.h>
#include <stdlib.h>

#include "../interpreter/wasm_runtime.h"
#include "../interpreter/wasm_opcode.h"
#include "wasm_dispatch.h"

#define skip_leb(p) while (*p++ & 0x80)

// *ipにopcodeが入ってる状態で引数に渡す
uint8* dispatch(uint8 *ip) {
    switch (*ip++) {
        case WASM_OP_CALL_INDIRECT:
        case WASM_OP_RETURN_CALL_INDIRECT:
            skip_leb(ip);
            skip_leb(ip);
            break;
        /* control instructions */
        case EXT_OP_BLOCK:
        case WASM_OP_BLOCK:
        case EXT_OP_LOOP:
        case WASM_OP_LOOP:
        case EXT_OP_IF:
        case WASM_OP_IF:
        case WASM_OP_BR:
        case WASM_OP_BR_IF:
        case WASM_OP_BR_TABLE:
        case WASM_OP_CALL:
        case WASM_OP_RETURN_CALL:
#if WASM_ENABLE_REF_TYPES != 0
        case WASM_OP_SELECT_T:
        case WASM_OP_TABLE_GET:
        case WASM_OP_TABLE_SET:
        case WASM_OP_REF_NULL:
        case WASM_OP_REF_FUNC:
#endif
        /* variable instructions */
        case WASM_OP_GET_LOCAL:
        case EXT_OP_GET_LOCAL_FAST:
        case WASM_OP_SET_LOCAL:
        case EXT_OP_SET_LOCAL_FAST:
        case WASM_OP_TEE_LOCAL:
        case EXT_OP_TEE_LOCAL_FAST:
        case WASM_OP_GET_GLOBAL:
        case WASM_OP_GET_GLOBAL_64:
        case WASM_OP_SET_GLOBAL:
        case WASM_OP_SET_GLOBAL_AUX_STACK:
        case WASM_OP_SET_GLOBAL_64:
            skip_leb(ip);
            break;
        /* memory load instructions */
        case WASM_OP_I32_LOAD:
        case WASM_OP_F32_LOAD:
        case WASM_OP_I64_LOAD:
        case WASM_OP_F64_LOAD:
        case WASM_OP_I32_LOAD8_S:
        case WASM_OP_I32_LOAD8_U:
        case WASM_OP_I32_LOAD16_S:
        case WASM_OP_I32_LOAD16_U:
        case WASM_OP_I64_LOAD8_S:
        case WASM_OP_I64_LOAD8_U:
        case WASM_OP_I64_LOAD16_S:
        case WASM_OP_I64_LOAD16_U:
        case WASM_OP_I64_LOAD32_S:
        case WASM_OP_I64_LOAD32_U:
        case WASM_OP_I32_STORE:
        case WASM_OP_F32_STORE:
        case WASM_OP_I64_STORE:
        case WASM_OP_F64_STORE:
        case WASM_OP_I32_STORE8:
        case WASM_OP_I32_STORE16:
        case WASM_OP_I64_STORE8:
        case WASM_OP_I64_STORE16:
        case WASM_OP_I64_STORE32:
        case WASM_OP_MEMORY_SIZE:
        case WASM_OP_MEMORY_GROW:
            skip_leb(ip);
            skip_leb(ip);
            break;
        /* constant instructions */
        case WASM_OP_I32_CONST:
        case WASM_OP_I64_CONST:
        case WASM_OP_F32_CONST:
        case WASM_OP_F64_CONST:
            skip_leb(ip);
            break;

        case WASM_OP_MISC_PREFIX:
            skip_leb(ip);
            switch (*ip++) {
#if WASM_ENABLE_BULK_MEMORY != 0
                case WASM_OP_MEMORY_INIT:
                case WASM_OP_DATA_DROP:
                case WASM_OP_MEMORY_COPY:
                case WASM_OP_MEMORY_FILL:
#endif /* WASM_ENABLE_BULK_MEMORY */
#if WASM_ENABLE_REF_TYPES != 0
                case WASM_OP_TABLE_INIT:
                case WASM_OP_ELEM_DROP:
                case WASM_OP_TABLE_COPY:
                case WASM_OP_TABLE_GROW:
                case WASM_OP_TABLE_SIZE:
                case WASM_OP_TABLE_FILL:
#endif /* WASM_ENABLE_REF_TYPES */
                    skip_leb(ip);
                default:
                    break;
            }
        
        case WASM_OP_ATOMIC_PREFIX:
            /* TODO */
            break;
        default:
            break;
    }
    return ip;
}