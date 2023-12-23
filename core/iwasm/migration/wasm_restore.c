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
        printf("file is %s\n", dir);
        perror("failed to openImg\n");
        return NULL;
    }

    return fp;
}

static inline WASMInterpFrame *
wasm_alloc_frame(WASMExecEnv *exec_env, uint32 size, WASMInterpFrame *prev_frame)
{
    WASMInterpFrame *frame = wasm_exec_env_alloc_wasm_frame(exec_env, size);

    if (frame) {
        frame->prev_frame = prev_frame;
#if WASM_ENABLE_PERF_PROFILING != 0
        frame->time_started = os_time_get_boot_microsecond();
#endif
    }
    else {
        wasm_set_exception((WASMModuleInstance *)exec_env->module_inst,
                           "wasm operand stack overflow");
    }

    return frame;
}

// static void
// restore_type_stack(WASMInterpFrame *frame, WASMExecEnv *exec_env, FILE *fp)
// {

//     WASMFunctionInstance *func = frame->function;

//     frame->tsp_bottom = frame->csp_boundary;
//     frame->tsp_boundary = frame->tsp_bottom + func->u.func->max_stack_cell_num;
//     uint32 tsp_offset;
//     fread(&tsp_offset, sizeof(uint32), 1, fp);
//     frame->tsp = frame->tsp_bottom + tsp_offset;

//     fread(frame->tsp_bottom, sizeof(uint32), tsp_offset, fp);
// }

static void
debug_frame(WASMInterpFrame* frame)
{
    // fprintf(stderr, "Return Address: (%d, %d)\n", fidx, offset);
    // fprintf(stderr, "TypeStack Content: [");
    // uint32* tsp_bottom = frame->tsp_bottom;
    // for (uint32 i = 0; i < type_stack_size; ++i) {
    //     uint8 type = *(tsp_bottom+i);
    //     fprintf(stderr, "%d, ", type);
    // }
    // fprintf(stderr, "]\n");
    // fprintf(stderr, "Value Stack Size: %d\n", value_stack_size);
    // fprintf(stderr, "Type Stack Size(Local含む): %d\n", full_type_stack_size);
    // fprintf(stderr, "Type Stack Size(Local含まず): %d\n", type_stack_size);
    // fprintf(stderr, "Label Stack Size: %d\n", ctrl_stack_size);
    
}

static void
debug_local(WASMInterpFrame *frame)
{
    WASMFunctionInstance *func = frame->function;
    uint32 *lp = frame->lp;
    uint32 param_count = func->param_count;
    uint32 local_count = func->local_count;

    fprintf(stderr, "locals: [");
    for (uint32 i = 0; i < param_count; i++) {
        switch (func->param_types[i]) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
                fprintf(stderr, "%u, ", *(uint32 *)lp);
                lp++;
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                fprintf(stderr, "%lu, ", *(uint64 *)lp);
                lp += 2;
                break;
            default:
                printf("TYPE NULL\n");
                break;
        }
    }

    /* local */
    for (uint32 i = 0; i < local_count; i++) {
        switch (func->local_types[i]) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
                fprintf(stderr, "%u, ", *(uint32 *)lp);
                lp++;
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                fprintf(stderr, "%lu, ", *(uint64 *)lp);
                lp += 2;
                break;
            default:
                printf("TYPE NULL\n");
                break;
        }
    }
    fprintf(stderr, "]\n");
}

static uint32
get_addr_offset(void* target, void* base)
{
    if (target == NULL) return -1;
    else return target - base;
}

static void
debug_label_stack(WASMInterpFrame *frame)
{
    WASMBranchBlock *csp = frame->csp_bottom;
    uint32 csp_num = frame->csp - csp;
    
    fprintf(stderr, "label stack: [\n");
    for (int i = 0; i < csp_num; i++, csp++) {
        // uint8 *begin_addr;
        fprintf(stderr, "\t{%d",
            // csp->begin_addr == NULL ? -1 : csp->begin_addr - wasm_get_func_code(frame->function);
            get_addr_offset(csp->begin_addr, wasm_get_func_code(frame->function))
        );

        // uint8 *target_addr;
        fprintf(stderr, ", %d",
            get_addr_offset(csp->target_addr, wasm_get_func_code(frame->function))
        );

        // uint32 *frame_sp;
        fprintf(stderr, ", %d",
            get_addr_offset(csp->frame_sp, frame->sp_bottom)
        );

        // uint32 *frame_tsp
        fprintf(stderr, ", %d",
            get_addr_offset(csp->frame_tsp, frame->tsp_bottom)
        );

        // uint32 cell_num;
        fprintf(stderr, ", %d", csp->cell_num);

        // uint32 count;
        fprintf(stderr, ", %d}\n", csp->count);
    }
    fprintf(stderr, "]\n");
}

static void
restore_WASMInterpFrame(WASMInterpFrame *frame, WASMExecEnv *exec_env, FILE *fps[3])
{
    WASMModuleInstance *module_inst = exec_env->module_inst;
    WASMFunctionInstance *func = frame->function;
    FILE *fp = fps[0];
    FILE *fp2 = fps[1];
    FILE *tsp_fp = fps[2];

    // struct WASMInterpFrame *prev_frame;
    // struct WASMFunctionInstance *function;
    // uint8 *ip;
    uint32 ip_offset;
    fread(&ip_offset, sizeof(uint32), 1, fp);
    uint32 fidx = func - module_inst->e->functions;
    frame->ip = wasm_get_func_code(frame->function) + ip_offset;
    fprintf(stderr, "Return Address: (%d, %d)\n", fidx, ip_offset);

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
    fprintf(stderr, "sp_offset: %d\n", sp_offset);


    // WASMBranchBlock *csp_bottom;
    // WASMBranchBlock *csp_boundary;
    // WASMBranchBlock *csp;
    frame->csp_bottom = frame->sp_boundary;
    frame->csp_boundary = frame->csp_bottom + func->u.func->max_block_num;
    uint32 csp_offset;
    fread(&csp_offset, sizeof(uint32), 1, fp);
    frame->csp = frame->csp_bottom + csp_offset;
    fprintf(stderr, "csp_offset: %d\n", csp_offset);
    
    
    // uint32 *tsp_bottom;
    // uint32 *tsp_boundary;
    // uint32 *tsp;
    frame->tsp_bottom = frame->csp_boundary;
    frame->tsp_boundary = frame->tsp_bottom + func->u.func->max_stack_cell_num;
    uint32 tsp_offset;
    fread(&tsp_offset, sizeof(uint32), 1, tsp_fp);
    frame->tsp = frame->tsp_bottom + tsp_offset;
    fprintf(stderr, "tsp_offset: %d\n", tsp_offset);

    // =========================================================

    // uint32 lp[1];
    uint32 *lp = frame->lp;
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
    debug_local(frame);

    fread(frame->sp_bottom, sizeof(uint32), sp_offset, fp);
    fread(frame->tsp_bottom, sizeof(uint32), tsp_offset, tsp_fp);

    WASMBranchBlock *csp = frame->csp_bottom;
    uint32 csp_num = frame->csp - frame->csp_bottom;

    uint64 addr;
    for (int i = 0; i < csp_num; i++, csp++) {
        // uint8 *begin_addr;
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

        // uint32 *frame_tsp
        // fread(&addr, sizeof(uint64), 1, fp2);
        fread(&addr, sizeof(uint64), 1, fp);
        if (addr == -1) {
            csp->frame_tsp = NULL;
        }
        else {
            csp->frame_tsp = addr + frame->tsp_bottom;
        }

        // uint32 cell_num;
        fread(&csp->cell_num, sizeof(uint32), 1, fp);
        // uint32 count;
        // fread(&csp->count, sizeof(uint32), 1, fp2);
        fread(&csp->count, sizeof(uint32), 1, fp);
    }
    debug_label_stack(frame);
}

WASMInterpFrame*
wasm_restore_frame(WASMExecEnv **_exec_env)
{
    WASMExecEnv *exec_env = *_exec_env;
    WASMModuleInstance *module_inst =
        (WASMModuleInstance *)exec_env->module_inst;
    WASMInterpFrame *frame, *prev_frame = wasm_exec_env_get_cur_frame(exec_env);
    WASMFunctionInstance *function;
    uint32 func_idx, frame_size, all_cell_num;
    FILE *fp;

    const char* img_dir = "";
    fp = openImg(img_dir, "frame_stack.img");
    if (fp == NULL) {
        perror("failed to open frame.img\n");
        return NULL;
    }

    // frameのcspのtsp
    FILE *csp_tsp_fp = openImg("", "ctrl_tsp.img");
    if (csp_tsp_fp == NULL) {
        perror("failed to open frame.img\n");
        return NULL;
    }

    // frameのtype stack
    FILE *tsp_fp = openImg("", "type_stack.img");
    if (tsp_fp == NULL) {
        perror("failed to open type_stack.img\n");
        return NULL;
    }
    FILE *fps[3] = {fp, csp_tsp_fp, tsp_fp};

    uint32 cnt = 0;
    while (!feof(fp)) {
        cnt++;
        if ((fread(&func_idx, sizeof(uint32), 1, fp)) == 0) {
            break;
        }

        if (func_idx == -1) {
            // 初期フレームのスタックサイズをreadしてALLOC
            fread(&all_cell_num, sizeof(uint32), 1, fp);
            frame_size = wasm_interp_interp_frame_size(all_cell_num);
            frame = prev_frame;

            // 初期フレームをrestore
            frame->function = NULL;
            frame->ip = NULL;
            frame->sp = prev_frame->lp + 0;
        }
        else {
            // 関数からスタックサイズを計算し,ALLOC
            function = module_inst->e->functions + func_idx;
            all_cell_num = (uint64)function->param_cell_num
                           + (uint64)function->local_cell_num
                           + (uint64)function->u.func->max_stack_cell_num
                           + ((uint64)function->u.func->max_block_num)
                                 * sizeof(WASMBranchBlock) / 4
                           + (uint64)function->u.func->max_stack_cell_num;
            frame_size = wasm_interp_interp_frame_size(all_cell_num);
            frame = wasm_alloc_frame(exec_env, frame_size,
                                (WASMInterpFrame *)prev_frame);

            // フレームをrestore
            frame->function = function;
            restore_WASMInterpFrame(frame, exec_env, fps);
        }
        prev_frame = frame;
        fprintf(stderr, "\n");
    }
    fprintf(stdout, "[DEBUG]restore_frame_count: %d\n", cnt);
    debug_wasm_interp_frame(frame, module_inst->e->functions);


    printf("Success to restore frame\n");
    wasm_exec_env_set_cur_frame(exec_env, frame);
    fclose(fp);
    fclose(csp_tsp_fp);
    fclose(tsp_fp);
    
    _exec_env = &exec_env;

    return frame;
}


static void
_restore_stack(WASMExecEnv *exec_env, WASMInterpFrame *frame, FILE *fp)
{
    WASMModuleInstance *module_inst = exec_env->module_inst;
    WASMFunctionInstance *func = frame->function;

    // 初期化
    frame->sp_bottom = frame->lp + func->param_cell_num + func->local_cell_num;
    frame->sp_boundary = frame->sp_bottom + func->u.func->max_stack_cell_num;
    frame->csp_bottom = frame->sp_boundary;
    frame->csp_boundary = frame->csp_bottom + func->u.func->max_block_num;
    frame->tsp_bottom = frame->csp_boundary;
    frame->tsp_boundary = frame->tsp_bottom + func->u.func->max_stack_cell_num;

    // リターンアドレス
    uint32 fidx, offset;
    fread(&fidx, sizeof(uint32), 1, fp);
    fread(&offset, sizeof(uint32), 1, fp);
    frame->function = module_inst->e->functions + fidx;
    frame->ip = wasm_get_func_code(frame->function) + offset;

    // 型スタックのサイズ
    uint32 locals = func->param_count + func->local_count;
    uint32 full_type_stack_size, type_stack_size;
    fread(&full_type_stack_size, sizeof(uint32), 1, fp);
    type_stack_size = full_type_stack_size - locals;                                      // 統一フォーマットでは、ローカルも型/値スタックに入れているが、WAMRの型/値スタックのサイズはローカル抜き
    frame->tsp = frame->tsp_bottom + type_stack_size;

    // 型スタックの中身
    fseek(fp, sizeof(uint8)*locals, SEEK_CUR);                      // localのやつはWAMRでは必要ないので飛ばす

    uint32* tsp_bottom = frame->tsp_bottom;
    for (uint32 i = 0; i < type_stack_size; ++i) {
        uint8 type;
        fread(&type, sizeof(uint8), 1, fp);
        *(tsp_bottom+i) = type;
    }
    // fread(frame->tsp_bottom, sizeof(uint8), type_stack_size, fp);

    // 値スタックのサイズ
    uint32 *tsp = frame->tsp_bottom;
    uint32 value_stack_size = 0;
    for (uint32 i = 0; i < type_stack_size; ++i, ++tsp) {
        value_stack_size += *tsp;
    }
    frame->sp = frame->sp_bottom + value_stack_size;

    // 値スタックの中身
    uint32 local_cell_num = func->param_cell_num + func->local_cell_num;
    fread(frame->lp, sizeof(uint32), local_cell_num, fp);
    // debug_local(frame);
    fread(frame->sp_bottom, sizeof(uint32), value_stack_size, fp);

    // ラベルスタックのサイズ
    uint32 ctrl_stack_size;
    fread(&ctrl_stack_size, sizeof(uint32), 1, fp);
    frame->csp = frame->csp_bottom + ctrl_stack_size;


    // ラベルスタックの中身
    WASMBranchBlock *csp = frame->csp_bottom;
    uint64 addr;
    for (int i = 0; i < ctrl_stack_size; ++i, ++csp) {
        // uint8 *begin_addr;
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

        // uint32 *frame_tsp
        // fread(&addr, sizeof(uint64), 1, fp2);
        fread(&addr, sizeof(uint64), 1, fp);
        if (addr == -1) {
            csp->frame_tsp = NULL;
        }
        else {
            csp->frame_tsp = addr + frame->tsp_bottom;
        }

        // uint32 cell_num;
        fread(&csp->cell_num, sizeof(uint32), 1, fp);
        // uint32 count;
        // fread(&csp->count, sizeof(uint32), 1, fp2);
        fread(&csp->count, sizeof(uint32), 1, fp);
    }
}

WASMInterpFrame*
wasm_restore_stack(WASMExecEnv **_exec_env)
{
    WASMExecEnv *exec_env = *_exec_env;
    WASMModuleInstance *module_inst =
        (WASMModuleInstance *)exec_env->module_inst;
    WASMInterpFrame *frame, *prev_frame = wasm_exec_env_get_cur_frame(exec_env);
    WASMFunctionInstance *function;
    uint32 func_idx, frame_size, all_cell_num;
    FILE *fp;

    uint32 frame_stack_size;
    fp = openImg("", "frame.img");
    if (fp == NULL) {
        perror("failed to open frame.img\n");
        return NULL;
    }
    fread(&frame_stack_size, sizeof(uint32), 1, fp);
    fclose(fp);

    char file[32];
    uint32 fidx = 0;
    for (uint32 i = 0; i < frame_stack_size; ++i) {
        sprintf(file, "stack%d.img", i);
        fp = openImg("", file);
        if (fp == NULL) {
            perror("failed to open frame.img\n");
            return NULL;
        }

        // TODO: dummyの保存復元って必要？
        if (i == 0) {
            // 初期フレームのスタックサイズをreadしてALLOC
            fread(&fidx, sizeof(uint32), 1, fp);
            fread(&all_cell_num, sizeof(uint32), 1, fp);
            frame_size = wasm_interp_interp_frame_size(all_cell_num);
            frame = prev_frame;

            // 初期フレームをrestore
            frame->function = NULL;
            frame->ip = NULL;
            frame->sp = prev_frame->lp + 0;
        }
        else {
            // 関数からスタックサイズを計算し,ALLOC
            // 前のframeのenter_func_idxが、このframe->functionに対応
            function = module_inst->e->functions + fidx;

            // TODO: uint64になってるけど、多分uint32
            all_cell_num = (uint64)function->param_cell_num
                           + (uint64)function->local_cell_num
                           + (uint64)function->u.func->max_stack_cell_num
                           + ((uint64)function->u.func->max_block_num)
                                 * sizeof(WASMBranchBlock) / 4
                           + (uint64)function->u.func->max_stack_cell_num;
            frame_size = wasm_interp_interp_frame_size(all_cell_num);
            frame = wasm_alloc_frame(exec_env, frame_size,
                                (WASMInterpFrame *)prev_frame);

            fread(&fidx, sizeof(uint32), 1, fp);
            // フレームをrestore
            frame->function = function;
            _restore_stack(exec_env, frame, fp);
        }

        prev_frame = frame;
        fclose(fp);
    }

    // debug_wasm_interp_frame(frame, module_inst->e->functions);
    wasm_exec_env_set_cur_frame(exec_env, frame);
    
    _exec_env = &exec_env;

    return frame;
}

int wasm_restore_memory(WASMModuleInstance *module, WASMMemoryInstance **memory) {
    FILE* memory_fp = openImg("", "memory.img");
    if (memory_fp == NULL) {
        perror("failed to openImg\n");
        return -1;
    }

    FILE* mem_size_fp = openImg("", "mem_page_count.img");
    if (mem_size_fp == NULL) {
        perror("failed to openImg\n");
        return -1;
    }
    // restore page_count
    uint32 page_count;
    fread(&page_count, sizeof(uint32), 1, mem_size_fp);
    wasm_enlarge_memory(module, page_count- (*memory)->cur_page_count);

    // restore memory_data
    fread((*memory)->memory_data, sizeof(uint8),
            (*memory)->num_bytes_per_page * (*memory)->cur_page_count, memory_fp);

    fclose(memory_fp);
    fclose(mem_size_fp);
    return 0;
}

int wasm_restore_global(const WASMModuleInstance *module, const WASMGlobalInstance *globals, uint8 **global_data, uint8 **global_addr) {
    const char *file = "global.img";
    FILE* fp = openImg("", file);
    if (fp == NULL) {
        perror("failed to openImg\n");
        return -1;
    }

    for (int i = 0; i < module->e->global_count; i++) {
        switch (globals[i].type) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
                *global_addr = get_global_addr_for_migration(*global_data, globals + i);
                fread(*global_addr, sizeof(uint32), 1, fp);
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                *global_addr = get_global_addr_for_migration(*global_data, globals + i);
                fread(*global_addr, sizeof(uint64), 1, fp);
                break;
            default:
                perror("wasm_restore_global:type error:A\n");
                break;
        }
    }

    fclose(fp);
    return 0;
}

void debug_addr(const char* name, const char* func_name, int value) {
    if (value == NULL) {
        fprintf(stderr, "debug_addr: %s value is NULL\n", name);
        return;
    }
    printf("%s in %s: %p\n", name, func_name, (int)value);
}

int wasm_restore_program_counter(
    WASMModuleInstance *module,
    uint8 **frame_ip)
{
    const char *file = "program_counter.img";
    FILE* fp = openImg("", file);
    if (fp == NULL) {
        perror("failed to openImg\n");
        return -1;
    }

    uint32 fidx, offset;
    fread(&fidx, sizeof(uint32), 1, fp);
    fread(&offset, sizeof(uint32), 1, fp);

    *frame_ip = wasm_get_func_code(module->e->functions + fidx) + offset;

    return 0;
}

int wasm_restore_addrs(
    const WASMInterpFrame *frame,
    const WASMFunctionInstance *func,
    const WASMMemoryInstance *memory,
    uint8 **frame_ip,
    uint32 **frame_lp,
    uint32 **frame_sp,
    WASMBranchBlock **frame_csp,
    uint8 **frame_ip_end,
    uint8 **else_addr,
    uint8 **end_addr,
    uint8 **maddr,
    bool *done_flag) 
{
    const char *file = "addr.img";
    FILE* fp = openImg("", file);
    if (fp == NULL) {
        perror("failed to openImg\n");
        return -1;
    }

    uint32 p_offset;
    *frame_lp = frame->lp;

    // register uint32 *frame_sp = NULL;
    fread(&p_offset, sizeof(uint32), 1, fp);
    *frame_sp = frame->sp_bottom + p_offset;

    // WASMBranchBlock *frame_csp = NULL;
    fread(&p_offset, sizeof(uint32), 1, fp);
    *frame_csp = frame->csp_bottom + p_offset;

    // uint8 *frame_ip_end = frame_ip + 1;
    *frame_ip_end = wasm_get_func_code_end(frame->function);

    // uint8 *else_addr, *end_addr, *maddr;
    fread(&p_offset, sizeof(uint32), 1, fp);
    *else_addr = wasm_get_func_code(func) + p_offset;

    fread(&p_offset, sizeof(uint32), 1, fp);
    *end_addr = wasm_get_func_code(func) + p_offset;

    fread(&p_offset, sizeof(uint32), 1, fp);
    *maddr = memory->memory_data + p_offset;

    fread(done_flag, sizeof(bool), 1, fp);

    fclose(fp);
    return 0;
}

int wasm_restore_tsp_addr(uint32 **frame_tsp, const WASMInterpFrame *frame)
{
    const char *file = "tsp_addr.img";
    FILE* fp = openImg("", file);
    if (fp == NULL) {
        fprintf(stderr, "failed to open %s\n", file);
        return -1;
    }

    uint32 p_offset;
    fread(&p_offset, sizeof(uint32), 1, fp);
    *frame_tsp = frame->tsp_bottom + p_offset;

    fclose(fp);
    return 0;
}

int wasm_restore(WASMModuleInstance **module,
            WASMExecEnv **exec_env,
            WASMFunctionInstance **cur_func,
            WASMInterpFrame **prev_frame,
            WASMMemoryInstance **memory,
            WASMGlobalInstance **globals,
            uint8 **global_data,
            uint8 **global_addr,
            WASMInterpFrame **frame,
            uint8 **frame_ip,
            uint32 **frame_lp,
            uint32 **frame_sp,
            WASMBranchBlock **frame_csp,
            uint8 **frame_ip_end,
            uint8 **else_addr,
            uint8 **end_addr,
            uint8 **maddr,
            bool *done_flag)
{
    // restore memory
    wasm_restore_memory(*module, memory);
    // printf("Success to restore linear memory\n");

    // restore globals
    wasm_restore_global(*module, *globals, global_data, global_addr);
    // printf("Success to restore globals\n");

    // restore program counter
    wasm_restore_program_counter(*module, frame_ip);
    // printf("Success to program counter\n");

    // restore addrs
    wasm_restore_addrs(*frame, *cur_func, *memory,
                        frame_ip, frame_lp, frame_sp, frame_csp,
                        frame_ip_end, else_addr, end_addr, maddr, done_flag);

    return 0;
}