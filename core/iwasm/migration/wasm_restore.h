#ifndef _WASM_RESTORE_H
#define _WASM_RESTORE_H

#include "common/wasm_exec_env.h"
#include "interpreter/wasm_interp.h"

WASMInterpFrame *
wasm_restore_frame(WASMExecEnv *exec_env, const char *dir);

static void
restore_WASMInterpFrame(WASMInterpFrame *frame, WASMExecEnv *exec_env,
                        FILE *fp);

#endif // _WASM_RESTORE_H