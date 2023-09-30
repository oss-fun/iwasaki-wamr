#ifndef _WASM_CHECKPOINT_H
#define _WASM_CHECKPOINT_H

#include "../common/wasm_exec_env.h"
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
