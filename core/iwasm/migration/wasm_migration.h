#ifndef _WASM_CHECKPOINT_H
#define _WASM_CHECKPOINT_H

// #include "../common/wasm_exec_env.h"
#include "../interpreter/wasm_interp.h"

static inline uint8 *
get_global_addr_for_migration(uint8 *global_data, WASMGlobalInstance *global)
{
#if WASM_ENABLE_MULTI_MODULE == 0
    return global_data + global->data_offset;
#else
    return global->import_global_inst
               ? global->import_module_inst->global_data
                     + global->import_global_inst->data_offset
               : global_data + global->data_offset;
#endif
}

static bool restore_flag = false;
inline void set_restore_flag(bool f) {
    restore_flag = f;
}
inline bool get_restore_flag() {
    return restore_flag;
}


int wasm_dump(WASMExecEnv *exec_env,
         WASMModuleInstance *module,
         WASMMemoryInstance *memory,
         WASMGlobalInstance *globals,
         uint8 *global_data,
         uint8 *global_addr,
         WASMFunctionInstance *cur_func,
         struct WASMInterpFrame *frame,
         register uint8 *frame_ip,
         register uint32 *frame_sp,
         WASMBranchBlock *frame_csp,
         uint8 *frame_ip_end,
         uint8 *else_addr,
         uint8 *end_addr,
         uint8 *maddr,
         bool done_flag);

int wasm_restore(WASMModuleInstance *module,
            WASMExecEnv *exec_env,
            WASMFunctionInstance *cur_func,
            WASMInterpFrame *prev_frame,
            WASMMemoryInstance *memory,
            WASMGlobalInstance *globals,
            uint8 *global_data,
            uint8 *global_addr,
            WASMInterpFrame *frame,
            register uint8 *frame_ip,
            register uint32 *frame_lp,
            register uint32 *frame_sp,
            WASMBranchBlock *frame_csp,
            uint8 *frame_ip_end,
            uint8 *else_addr,
            uint8 *end_addr,
            uint8 *maddr,
            bool *done_flag);
#endif // _WASM_CHECKPOINT_H
