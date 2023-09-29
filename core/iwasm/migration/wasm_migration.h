#ifndef _WASM_CHECKPOINT_H
#define _WASM_CHECKPOINT_H

#include "../common/wasm_exec_env.h"
#include "../interpreter/wasm_interp.h"

typedef struct Frame_Info {
    WASMInterpFrame *frame;
    uint32 all_cell_num;
    struct Frame_Info *prev;
    struct Frame_Info *next;
} Frame_Info;


#endif // _WASM_CHECKPOINT_H
