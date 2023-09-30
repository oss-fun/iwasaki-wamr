#ifndef _WASM_CHECKPOINT_H
#define _WASM_CHECKPOINT_H

#include "../common/wasm_exec_env.h"
#include "../interpreter/wasm_interp.h"

// void
// wasm_migration_alloc_frame(WASMInterpFrame *frame, WASMExecEnv *exec_env)
// {
//     Frame_Info *info;
//     info = malloc(sizeof(struct FrameInfo));
//     info->frame = frame;
//     info->all_cell_num = 0;
//     info->prev = tail_info;
//     info->next = NULL;

//     if (root_info == NULL) {
//         root_info = tail_info = info;
//     }
//     else {
//         tail_info->next = info;
//         tail_info = tail_info->next;
//     }
// }

// void
// wasm_migration_free_frame(void)
// {
//     if (root_info == tail_info) {
//         free(root_info);
//         root_info = tail_info = NULL;
//     }
//     else {
//         tail_info = tail_info->prev;
//         free(tail_info->next);
//         tail_info->next = NULL;
//     }
// }

#endif // _WASM_CHECKPOINT_H
