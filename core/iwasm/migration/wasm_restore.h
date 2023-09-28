#ifndef _WASM_RESTORE_H
#define _WASM_RESTORE_H

#include "common/wasm_exec_env.h"
#include "interpreter/wasm_interp.h"

WASMInterpFrame *
wasm_restore_frame(WASMExecEnv *exec_env, const char *dir);

static void
restore_WASMInterpFrame(WASMInterpFrame *frame, WASMExecEnv *exec_env,
                        FILE *fp);

int restore(WASMModuleInstance *module,
            WASMExecEnv *exec_env,
            WASMMemoryInstance *memory,
            WASMGlobalInstance *globals,
            register uint8 *frame_ip,
            register uint32 *frame_lp,
            register uint32 *frame_sp,
            WASMBranchBlock *frame_csp,
            uint8 *frame_ip_end,
            uint8 *else_addr,
            uint8 *end_addr,
            uint8 *maddr,
            bool *done_flag);
#endif // _WASM_RESTORE_H