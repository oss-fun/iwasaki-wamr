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

void
wasm_dump_set_root_and_tail(Frame_Info *root, Frame_Info *tail);

void
wasm_dump_alloc_init_frame(uint32 all_cell_num);

void
wasm_dump_alloc_frame(WASMInterpFrame *frame, WASMExecEnv *exec_env);

void
wasm_dump_free_frame(void);

void
wasm_dump_frame(WASMExecEnv *exec_env);

static void
dump_WASMInterpFrame(WASMInterpFrame *frame, WASMExecEnv *exec_env, FILE *fp);

int wasm_dump(WASMExecEnv *exec_env,
         WASMMemoryInstance *meomry,
         WASMGlobalInstance *globals,
         WASMFunctionInstance *cur_func,
         WASMInterpFrame *frame,
         register uint8 *frame_ip,
         register uint32 *frame_sp,
         WASMBranchBlock *frame_csp,
         uint8 *frame_ip_end,
         uint8 *else_addr,
         uint8 *end_addr,
         uint8 *maddr,
         bool done_flag);

#endif // _WASM_CHECKPOINT_H
