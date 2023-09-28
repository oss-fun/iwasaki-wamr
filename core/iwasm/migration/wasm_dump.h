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

int dump(const WASMExecEnv *exec_env,
         const WASMMemoryInstance *meomry,
         const WASMGlobalInstance *globals,
         const WASMFunctionInstance *cur_func,
         const WASMInterpFrame *frame,
         const register uint8 *frame_ip,
         const register uint32 *frame_sp,
         const WASMBranchBlock *frame_csp,
         const uint8 *frame_ip_end,
         const uint8 *else_addr,
         const uint8 *end_addr,
         const uint8 *maddr,
         const bool done_flag);

#endif // _WASM_CHECKPOINT_H
