#include <stdio.h>
#include <stdlib.h>

#include "../common/wasm_exec_env.h"
#include "../common/wasm_memory.h"
#include "../interpreter/wasm_runtime.h"
#include "wasm_migration.h"
#include "wasm_restore.h"

static bool restore_flag;
void set_restore_flag(bool f)
{
    restore_flag = f;
}
bool get_restore_flag()
{
    return restore_flag;
}

FILE* openImg(const char* img_dir, const char* file_path) {
    FILE *fp;

    char *dir = malloc((img_dir == NULL ? 0 : strlen(img_dir))
                        + strlen(file_path) + 1);
    if (img_dir != NULL) {
        dir = strcpy(dir, img_dir);
    }

    dir = strcat(dir, file_path);
    fp = fopen(dir, "rb");
    if (fp == NULL) {
        perror("failed to openImg\n");
        return NULL;
    }

    return fp;
}

// NOTE: もとのやつはprev_frameを引数に渡してそれを、新しく生成したframeのprev_frameにつけてたけど,
//       これはframeを引数で渡して新しくprev_frameを生成し、渡されたframeのprev_frameにつける
//       返すのはprev_frame
static inline WASMInterpFrame *
wasm_alloc_frame(WASMExecEnv *exec_env, uint32 size, WASMInterpFrame *frame)
{
    WASMInterpFrame *prev_frame = wasm_exec_env_alloc_wasm_frame(exec_env, size);

    if (frame) {
        frame->prev_frame = prev_frame;
#if WASM_ENABLE_PERF_PROFILING != 0
        prev_frame->time_started = os_time_get_boot_microsecond();
#endif
    }
    else {
        wasm_set_exception((WASMModuleInstance *)exec_env->module_inst,
                           "wasm operand stack overflow");
    }

    return prev_frame;
}

static void
restore_WASMInterpFrame(WASMInterpFrame *frame, WASMExecEnv *exec_env, FILE *fp)
{
    WASMModuleInstance *module_inst = exec_env->module_inst;
    WASMFunctionInstance *func = frame->function;

    // struct WASMInterpFrame *prev_frame;
    // struct WASMFunctionInstance *function;
    // uint8 *ip;
    uint32 ip_offset;
    fread(&ip_offset, sizeof(uint32), 1, fp);
    frame->ip = wasm_get_func_code(frame->function) + ip_offset;

    // uint32 *sp_bottom;
    // uint32 *sp_boundary;
    // uint32 *sp;
    // NOTE: sp_bottom = lp + param_cell_num + local_cell_num   x
    //                 = lp + param_cell_size + local_cell_size o (because not only 1byte but also 2byte)
    frame->sp_bottom = frame->lp + func->param_cell_num + func->local_cell_num;
    frame->sp_boundary = frame->sp_bottom + func->u.func->max_stack_cell_num;
    uint32 sp_offset;
    fread(&sp_offset, sizeof(uint32), 1, fp);
    frame->sp = frame->sp_bottom + sp_offset;


    // WASMBranchBlock *csp_bottom;
    // WASMBranchBlock *csp_boundary;
    // WASMBranchBlock *csp;
    frame->csp_bottom = frame->sp_boundary;
    frame->csp_boundary = frame->csp_bottom + func->u.func->max_block_num;
    uint32 csp_offset;
    fread(&csp_offset, sizeof(uint32), 1, fp);
    frame->csp = frame->csp_bottom + csp_offset;

    // =========================================================

    // uint32 lp[1];
    uint32 *lp = frame->lp;
    // VALUE_TYPE_I32
    // VALUE_TYPE_F32
    // VALUE_TYPE_I64
    // VALUE_TYPE_F64
    for (int i = 0; i < func->param_count; i++) {
        switch (func->param_types[i]) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
                fread(lp, sizeof(uint32), 1, fp);
                lp++;
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                fread(lp, sizeof(uint64), 1, fp);
                lp += 2;
                break;
            default:
                printf("TYPE NULL\n");
                break;
        }
    }
    for (int i = 0; i < func->local_count; i++) {
        switch (func->local_types[i]) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
                fread(lp, sizeof(uint32), 1, fp);
                lp++;
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                fread(lp, sizeof(uint64), 1, fp);
                lp += 2;
                break;
            default:
                printf("TYPE NULL\n");
                break;
        }
    }

    fread(frame->sp_bottom, sizeof(uint32), sp_offset, fp);

    WASMBranchBlock *csp = frame->csp_bottom;
    uint32 csp_num = frame->csp - frame->csp_bottom;

    for (int i = 0; i < csp_num; i++, csp++) {
        // uint8 *begin_addr;
        uint64 addr;
        fread(&addr, sizeof(uint64), 1, fp);
        if (addr == -1) {
            csp->begin_addr = NULL;
        }
        else {
            csp->begin_addr = addr + wasm_get_func_code(frame->function);
        }

        // uint8 *target_addr;
        fread(&addr, sizeof(uint64), 1, fp);
        if (addr == -1) {
            csp->target_addr = NULL;
        }
        else {
            csp->target_addr = addr + wasm_get_func_code(frame->function);
        }

        // uint32 *frame_sp;
        fread(&addr, sizeof(uint64), 1, fp);
        if (addr == -1) {
            csp->frame_sp = NULL;
        }
        else {
            csp->frame_sp = addr + frame->sp_bottom;
        }

        // uint32 cell_num;
        fread(&csp->cell_num, sizeof(uint32), 1, fp);
    }
}

// TODO: dump時にframeを逆順にrestoreしたかもなので、よく確認する
WASMInterpFrame *
wasm_restore_frame(WASMExecEnv *exec_env, const char *img_dir)
{
    WASMModuleInstance *module_inst =
        (WASMModuleInstance *)exec_env->module_inst;
    WASMInterpFrame *frame, *prev_frame = wasm_exec_env_get_cur_frame(exec_env);
    WASMFunctionInstance *function;
    uint32 func_idx, frame_size, all_cell_num;
    FILE *fp;

    fp = openImg(img_dir, "frame.img");
    if (fp == NULL) {
        perror("failed to open frame.img\n");
        return NULL;
    }
    assert(fp != NULL);

    while (!feof(fp)) {
        if ((fread(&func_idx, sizeof(uint32), 1, fp)) == 0) {
            break;
        }

        if (func_idx == -1) {
            // 初期フレームのスタックサイズをreadしてALLOC
            fread(&all_cell_num, sizeof(uint32), 1, fp);
            frame_size = wasm_interp_interp_frame_size(all_cell_num);
            frame = wasm_alloc_frame(exec_env, frame_size,
                                (WASMInterpFrame *)prev_frame);

            // 初期フレームをrestore
            frame->function = NULL;
            frame->ip = NULL;
            frame->sp = frame->lp + 0;

            // NOTE: info使う必要ないはずだから一旦コメントアウト
            //       多分もう一度dumpするときのためにinfoを作ってる気がする
            // info->frame = prev_frame = frame;
            // info->all_cell_num = all_cell_num;
        }
        else {
            // 関数からスタックサイズを計算し,ALLOC
            function = module_inst->e->functions + func_idx;
            printf("restore func_idx: %d\n", func_idx);

            all_cell_num = (uint64)function->param_cell_num
                           + (uint64)function->local_cell_num
                           + (uint64)function->u.func->max_stack_cell_num
                           + ((uint64)function->u.func->max_block_num)
                                 * sizeof(WASMBranchBlock) / 4;
            frame_size = wasm_interp_interp_frame_size(all_cell_num);
            frame = wasm_alloc_frame(exec_env, frame_size,
                                (WASMInterpFrame *)prev_frame);

            // フレームをrestore
            frame->function = function;
            restore_WASMInterpFrame(frame, exec_env, fp);

            printf("prev_frame:%p\n", prev_frame);

            // info->frame = prev_frame = frame;
        }

        // TODO: 関数化できない？
        // if (root_info == NULL) {
        //     info->next = info->prev = NULL;
        //     root_info = tail_info = info;
        // }
        // else {
        //     info->next = NULL;
        //     info->prev = tail_info;
        //     tail_info->next = info;
        //     tail_info = tail_info->next;
        // }
    }
    wasm_exec_env_set_cur_frame(exec_env, frame);
    // wasm_dump_set_root_and_tail(root_info, tail_info);
    fclose(fp);

    // TODO: このframeがちゃんと末尾のframeになってるかチェック
    return frame;
}




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
            bool *done_flag) 
{
    const char* img_dir = "";
    // WASMInterpFrame *frame = NULL;
    frame = wasm_restore_frame(exec_env, img_dir);
    if (frame == NULL) {
        perror("failed to wasm_restore_frame\n");
        return -1;
    }
    assert(frame != NULL);

    cur_func = frame->function;
    prev_frame = frame->prev_frame;


    FILE* fp = openImg(img_dir, "interp.img");
    if (fp == NULL) {
        perror("failed to openImg\n");
        return -1;
    }
    assert(fp != NULL);

    // WASMMemoryInstance *memory = module->default_memory;
    fread(memory->memory_data, sizeof(uint8),
            memory->num_bytes_per_page * memory->cur_page_count, fp);

    // uint8 *global_data = module->global_data;
    for (int i = 0; i < module->e->global_count; i++) {
        switch (globals[i].type) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
                global_addr = get_global_addr_for_migration(global_data, globals + i);
                fread(global_addr, sizeof(uint32), 1, fp);
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                global_addr = get_global_addr_for_migration(global_data, globals + i);
                fread(global_addr, sizeof(uint64), 1, fp);
                break;
            default:
                printf("type error:A\n");
                break;
        }
    }


    uint32 p_offset;
    // register uint8 *frame_ip = &opcode_IMPDEP;
    fread(&p_offset, sizeof(uint32), 1, fp);
    frame_ip = wasm_get_func_code(frame->function) + p_offset;

    // register uint32 *frame_lp = NULL;
    frame_lp = frame->lp;
    // register uint32 *frame_sp = NULL;
    fread(&p_offset, sizeof(uint32), 1, fp);
    frame_sp = frame->sp_bottom + p_offset;

    // WASMBranchBlock *frame_csp = NULL;
    fread(&p_offset, sizeof(uint32), 1, fp);
    frame_csp = frame->csp_bottom + p_offset;

    // uint8 *frame_ip_end = frame_ip + 1;
    frame_ip_end = wasm_get_func_code_end(frame->function);

    // uint8 *else_addr, *end_addr, *maddr;
    fread(&p_offset, sizeof(uint32), 1, fp);
    else_addr = wasm_get_func_code(cur_func) + p_offset;

    fread(&p_offset, sizeof(uint32), 1, fp);
    end_addr = wasm_get_func_code(cur_func) + p_offset;

    fread(&p_offset, sizeof(uint32), 1, fp);
    maddr = memory->memory_data + p_offset;

    fread(done_flag, sizeof(bool), 1, fp);

    // SYNC_ALL_TO_FRAME();

    fclose(fp);

    return 0;
}