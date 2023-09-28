#include <stdio.h>
#include <stdlib.h>

#include "interpreter/wasm_runtime.h"
#include "common/wasm_dump.h"

static Frame_Info *root_info = NULL, *tail_info = NULL;

void
wasm_dump_set_root_and_tail(Frame_Info *root, Frame_Info *tail)
{
    root_info = root;
    tail_info = tail;
}

void
wasm_dump_alloc_init_frame(uint32 all_cell_num)
{
    root_info->all_cell_num = all_cell_num;
}

void
wasm_dump_alloc_frame(WASMInterpFrame *frame, WASMExecEnv *exec_env)
{
    Frame_Info *info;
    info = malloc(sizeof(Frame_Info));
    info->frame = frame;
    info->all_cell_num = 0;
    info->prev = tail_info;
    info->next = NULL;

    if (root_info == NULL) {
        root_info = tail_info = info;
    }
    else {
        tail_info->next = info;
        tail_info = tail_info->next;
    }
}

void
wasm_dump_free_frame(void)
{
    if (root_info == tail_info) {
        free(root_info);
        root_info = tail_info = NULL;
    }
    else {
        tail_info = tail_info->prev;
        free(tail_info->next);
        tail_info->next = NULL;
    }
}

void
wasm_dump_frame(WASMExecEnv *exec_env)
{
    WASMModuleInstance *module_inst =
        (WASMModuleInstance *)exec_env->module_inst;
    Frame_Info *info;
    WASMFunctionInstance *function;
    uint32 func_idx;
    FILE *fp;
    int i;

    if (!root_info) {
        printf("dump failed\n");
        exit(1);
    }
    fp = fopen("frame.img", "wb");
    info = root_info;

    while (info) {

        if (info->frame->function == NULL) {
            // 初期フレーム
            func_idx = -1;
            fwrite(&func_idx, sizeof(uint32), 1, fp);
            fwrite(&info->all_cell_num, sizeof(uint32), 1, fp);
        }
        else {
            func_idx = info->frame->function - module_inst->functions;
            fwrite(&func_idx, sizeof(uint32), 1, fp);
            printf("dump func_idx: %d\n", func_idx);

            dump_WASMInterpFrame(info->frame, exec_env, fp);
        }
        info = info->next;
    }
    fclose(fp);
}

static void
dump_WASMInterpFrame(WASMInterpFrame *frame, WASMExecEnv *exec_env, FILE *fp)
{
    int i;
    WASMModuleInstance *module_inst = exec_env->module_inst;

    // struct WASMInterpFrame *prev_frame;
    // struct WASMFunctionInstance *function;
    // uint8 *ip;
    uint32 ip_offset = frame->ip - wasm_get_func_code(frame->function);
    fwrite(&ip_offset, sizeof(uint32), 1, fp);

    // uint32 *sp_bottom;
    // uint32 *sp_boundary;
    // uint32 *sp;
    uint32 sp_offset = frame->sp - frame->sp_bottom;
    fwrite(&sp_offset, sizeof(uint32), 1, fp);

    // WASMBranchBlock *csp_bottom;
    // WASMBranchBlock *csp_boundary;
    // WASMBranchBlock *csp;
    uint32 csp_offset = frame->csp - frame->csp_bottom;
    fwrite(&csp_offset, sizeof(uint32), 1, fp);

    // uint32 lp[1];
    WASMFunctionInstance *func = frame->function;

    uint32 *lp = frame->lp;
    // VALUE_TYPE_I32
    // VALUE_TYPE_F32
    // VALUE_TYPE_I64
    // VALUE_TYPE_F64
    for (i = 0; i < func->param_count; i++) {
        switch (func->param_types[i]) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
                fwrite(lp, sizeof(uint32), 1, fp);
                lp++;
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                fwrite(lp, sizeof(uint64), 1, fp);
                lp += 2;
                break;
            default:
                printf("TYPE NULL\n");
                break;
        }
    }
    for (i = 0; i < func->local_count; i++) {
        switch (func->local_types[i]) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
                fwrite(lp, sizeof(uint32), 1, fp);
                lp++;
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                fwrite(lp, sizeof(uint64), 1, fp);
                lp += 2;
                break;
            default:
                printf("TYPE NULL\n");
                break;
        }
    }

    fwrite(frame->sp_bottom, sizeof(uint32), sp_offset, fp);

    WASMBranchBlock *csp = frame->csp_bottom;
    uint32 csp_num = frame->csp - frame->csp_bottom;
    

    for (i = 0; i < csp_num; i++, csp++) {
        // uint8 *begin_addr;
        uint64 addr;
        if (csp->begin_addr == NULL) {
            addr = -1;
            fwrite(&addr, sizeof(uint64), 1, fp);
        }
        else {
            addr = csp->begin_addr - wasm_get_func_code(frame->function);
            fwrite(&addr, sizeof(uint64), 1, fp);
        }

        // uint8 *target_addr;
        if (csp->target_addr == NULL) {
            addr = -1;
            fwrite(&addr, sizeof(uint64), 1, fp);
        }
        else {
            addr = csp->target_addr - wasm_get_func_code(frame->function);
            fwrite(&addr, sizeof(uint64), 1, fp);
        }

        // uint32 *frame_sp;
        if (csp->frame_sp == NULL) {
            addr = -1;
            fwrite(&addr, sizeof(uint64), 1, fp);
        }
        else {
            addr = csp->frame_sp - frame->sp_bottom;
            fwrite(&addr, sizeof(uint64), 1, fp);
        }

        // uint32 cell_num;
        fwrite(&csp->cell_num, sizeof(uint32), 1, fp);
    }
}
