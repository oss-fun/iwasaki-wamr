#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../interpreter/wasm_runtime.h"
#include "wasm_migration.h"
#include "wasm_dump.h"
#include "wasm_dispatch.h"

#include "sgx_tprotected_fs.h"



#define BH_PLATFORM_LINUX 0

// #define skip_leb(p) while (*p++ & 0x80)
#define skip_leb(p)                     \
    while (1) {                         \
        if (*p & 0x80)p++;              \
        else break;                     \
    }                                   \

int64_t get_time(struct timespec ts1, struct timespec ts2) {
  int64_t sec = ts2.tv_sec - ts1.tv_sec;
  int64_t nsec = ts2.tv_nsec - ts1.tv_nsec;
  // std::cerr << sec << ", " << nsec << std::endl;
  return sec * 1e9 + nsec;
}

/* common_functions */
int dump_value(void *ptr, size_t size, size_t nmemb, SGX_FILE *stream) {
    if (stream == NULL) {
        return -1;
    }
    return sgx_fwrite(ptr, size, nmemb, stream);
}

int debug_memories(WASMModuleInstance *module) {
    // printf("=== debug memories ===\n");
    // printf("memory_count: %d\n", module->memory_count);
    
    
    // // bytes_per_page
    // for (int i = 0; i < module->memory_count; i++) {
    //     WASMMemoryInstance *memory = (WASMMemoryInstance *)(module->memories[i]);
    //     printf("%d) bytes_per_page: %d\n", i, memory->num_bytes_per_page);
    //     printf("%d) cur_page_count: %d\n", i, memory->cur_page_count);
    //     printf("%d) max_page_count: %d\n", i, memory->max_page_count);
    //     printf("\n");
    // }

    // printf("=== debug memories ===\n");
    return 0;
}

// 積まれてるframe stackを出力する
void debug_frame_info(WASMExecEnv* exec_env, WASMInterpFrame *frame) {
    WASMModuleInstance *module = exec_env->module_inst;

    // int cnt = 0;
    // printf("=== DEBUG Frame Stack ===\n");
    // do {
    //     cnt++;
    //     if (frame->function == NULL) {
    //         printf("%d) func_idx: -1\n", cnt);
    //     }
    //     else {
    //         printf("%d) func_idx: %d\n", cnt, frame->function - module->e->functions);
    //     }
    // } while (frame = frame->prev_frame);
    // printf("=== DEBUG Frame Stack ===\n");
}

// // func_instの先頭からlimitまでのopcodeを出力する
// int debug_function_opcodes(WASMModuleInstance *module, WASMFunctionInstance* func, uint32 limit) {
//     SGX_FILE *fp = sgx_fopen_auto_key("wamr_opcode.log", "a");
//     if (fp == NULL) return -1;

//     fprintf(fp, "fidx: %d\n", func - module->e->functions);
//     uint8 *ip = wasm_get_func_code(func);
//     uint8 *ip_end = wasm_get_func_code_end(func);
    
//     for (int i = 0; i < limit; i++) {
//         fprintf(fp, "%d) opcode: 0x%x\n", i+1, *ip);
//         ip = dispatch(ip, ip_end);
//         if (ip >= ip_end) break;
//     }

//     sgx_fclose(fp);
//     return 0;
// }

// int debug_flag = 0;
// ipからip_limまでにopcodeがいくつかるかを返す
int get_opcode_offset(uint8 *ip, uint8 *ip_lim) {
    uint32 cnt = 0;
    bh_assert(ip != NULL);
    bh_assert(ip_lim != NULL);
    bh_assert(ip <= ip_lim);
    if (ip > ip_lim) return -1;
    if (ip == ip_lim) return 0;
    while (1) {
        // LOG_DEBUG("get_opcode_offset::ip: 0x%x\n", *ip);
        // if (debug_flag) {
        //     printf("(cnt, opcode) = (%d, 0x%x)\n", cnt, *ip);
        // }
        ip = dispatch(ip, ip_lim);
        cnt++;
        if (ip >= ip_lim) break;
    }
    return cnt;
}

// TODO: コードごちゃごちゃで読めないので、整理する
uint8* get_type_stack(uint32 fidx, uint32 offset, uint32* type_stack_size, bool is_return_address) {
    
    
    SGX_FILE *tablemap_func = open_image("tablemap_func_sgx", "rwb");
    // sgx_fwrite(&ta, sizeof(ta), 1, tablemap_func);

    // SGX_FILE *tablemap_func = open_image("tablemap_func", "rb");
    open_type_stack("tablemap_func", "wb+");


    // if (!tablemap_func) ocall_print("not found tablemap_func\n");
    SGX_FILE *tablemap_offset = open_image("tablemap_offset", "rb");
    // if (!tablemap_offset) ocall_print("not found tablemap_offset\n");
    SGX_FILE *type_table = open_image("type_table", "rb");
    // if (!type_table) ocall_print("not found type_table\n");
    
    // tablemap_func
    sgx_fseek(tablemap_func, fidx*sizeof(uint32)*3, SEEK_SET);
    uint32 ffidx;
    uint64 tablemap_offset_addr;
    sgx_fread(&ffidx, sizeof(uint32), 1, tablemap_func);
    if (fidx != ffidx) {
        // perror("tablemap_funcがおかしい\n");
        // exit(1);

        // ocall_perror("dump.c 129\n");
        // ocall_exit(1);
    }
    sgx_fread(&tablemap_offset_addr, sizeof(uint64), 1, tablemap_func);

    /// tablemap_offset
    sgx_fseek(tablemap_offset, tablemap_offset_addr, SEEK_SET);
    // 関数fidxのローカルを取得
    uint32 locals_size;
    sgx_fread(&locals_size, sizeof(uint32), 1, tablemap_offset);
    uint8 locals[locals_size];
    sgx_fread(locals, sizeof(uint8), locals_size, tablemap_offset);
    // 対応するoffsetまで移動
    uint32 ooffset;
    uint64 type_table_addr, pre_type_table_addr;
    while(!sgx_feof(tablemap_offset)) {
       sgx_fread(&ooffset, sizeof(uint32), 1, tablemap_offset); 
       sgx_fread(&type_table_addr, sizeof(uint64), 1, tablemap_offset); 
       if (offset == ooffset) break;
       pre_type_table_addr = type_table_addr;
    }
    if (sgx_feof(tablemap_offset)) {
        // perror("tablemap_offsetがおかしい\n");
        // exit(1);

        // ocall_perror("dump.c 152\n");
        // ocall_exit(1);
    }
    // type_table_addr = pre_type_table_addr;

    /// type_table
    sgx_fseek(type_table, type_table_addr, SEEK_SET);
    uint32 stack_size;
    sgx_fread(&stack_size, sizeof(uint32), 1, type_table);
    uint8 stack[stack_size];
    sgx_fread(stack, sizeof(uint8), stack_size, type_table);

    if (is_return_address) {
        sgx_fread(&stack_size, sizeof(uint32), 1, type_table);
        sgx_fread(stack, sizeof(uint8), stack_size, type_table);
    }

    // uint8 type_stack[locals_size + stack_size];
    uint8* type_stack = malloc(locals_size + stack_size);
    for (uint32 i = 0; i < locals_size; ++i) type_stack[i] = locals[i];
    for (uint32 i = 0; i < stack_size; ++i) type_stack[locals_size + i] = stack[i];

    // printf("new type stack: [");
    // for (uint32 i = 0; i < locals_size + stack_size; ++i) {
    //     if (i+1 == locals_size + stack_size)printf("%d", type_stack[i]);
    //     else                                printf("%d, ", type_stack[i]);
    // }
    // printf("]\n");

    sgx_fclose(tablemap_func);
    sgx_fclose(tablemap_offset);
    sgx_fclose(type_table);

    *type_stack_size = locals_size + stack_size;
    return type_stack;
}

/* wasm_dump */
static void
_dump_stack(WASMExecEnv *exec_env, struct WASMInterpFrame *frame, SGX_FILE *fp, bool is_top)
{
    int i;
    WASMModuleInstance *module = exec_env->module_inst;

    // Entry function
    // wasm_dump_stackの方でdump

    // リターンアドレス
    // NOTE: 1番下のframeのときだけ、prev_frameではなくframeのリターンアドレスを出力する
    WASMInterpFrame* prev_frame = (frame->prev_frame->function ? frame->prev_frame : frame);
    uint32 fidx = prev_frame->function - module->e->functions;
    uint32 offset = prev_frame->ip - wasm_get_func_code(prev_frame->function);
    sgx_fwrite(&fidx, sizeof(uint32), 1, fp);
    sgx_fwrite(&offset, sizeof(uint32), 1, fp);

    // 型スタックのサイズ
    WASMFunctionInstance *func = frame->function;
    uint32 locals = func->param_count + func->local_count;
    // uint32 type_stack_size = (frame->tsp - frame->tsp_bottom);
    // uint32 full_type_stack_size = type_stack_size + locals;
    // sgx_fwrite(&full_type_stack_size, sizeof(uint32), 1, fp);

    // 型スタックの中身
    uint32 type_stack_size_from_file;
    uint32 fidx_now = frame->function - module->e->functions;
    uint32 offset_now = frame->ip - wasm_get_func_code(frame->function);
    // printf("[DEBUG]now addr: (%d, %d)\n", fidx_now, offset_now);
    uint8* type_stack_from_file = get_type_stack(fidx_now, offset_now, &type_stack_size_from_file, !is_top);
    sgx_fwrite(&type_stack_size_from_file, sizeof(uint32), 1, fp);
    sgx_fwrite(type_stack_from_file, sizeof(uint8), type_stack_size_from_file, fp);
    free(type_stack_from_file);

    // 値スタックの中身
    uint32 local_cell_num = func->param_cell_num + func->local_cell_num;
    uint32 value_stack_size = frame->sp - frame->sp_bottom;
    sgx_fwrite(frame->lp, sizeof(uint32), local_cell_num, fp);
    sgx_fwrite(frame->sp_bottom, sizeof(uint32), value_stack_size, fp);

    // ラベルスタックのサイズ
    uint32 ctrl_stack_size = frame->csp - frame->csp_bottom;
    sgx_fwrite(&ctrl_stack_size, sizeof(uint32), 1, fp);

    // ラベルスタックの中身
    WASMBranchBlock *csp = frame->csp_bottom;
    uint32 addr;
    uint8* ip_start = wasm_get_func_code(frame->function);
    for (i = 0; i < ctrl_stack_size; ++i, ++csp) {
        // uint8 *begin_addr;
        addr = get_addr_offset(csp->begin_addr, ip_start);
        sgx_fwrite(&addr, sizeof(uint32), 1, fp);

        // uint8 *target_addr;
        addr = get_addr_offset(csp->target_addr, ip_start);
        sgx_fwrite(&addr, sizeof(uint32), 1, fp);

        // uint32 *frame_sp;
        addr = get_addr_offset(csp->frame_sp, frame->sp_bottom);
        sgx_fwrite(&addr, sizeof(uint32), 1, fp);

        // uint32 *frame_tsp;
        // addr = get_addr_offset(csp->frame_tsp, frame->tsp_bottom);
        // sgx_fwrite(&addr, sizeof(uint32), 1, fp);
        
        // uint32 cell_num;
        sgx_fwrite(&csp->cell_num, sizeof(uint32), 1, fp);

        // uint32 count;
        // sgx_fwrite(&csp->count, sizeof(uint32), 1, fp);
    }
}


int
wasm_dump_stack(WASMExecEnv *exec_env, struct WASMInterpFrame *frame)
{
    WASMModuleInstance *module =
        (WASMModuleInstance *)exec_env->module_inst;

    // frameをtopからbottomまで走査する
    char file[32];
    int64_t i = 0;
    do {
        // dummy framenならbreak
        if (frame->function == NULL) break;

        ++i;
        snprintf(file, sizeof(file),"stack%d.img", i);
        SGX_FILE *fp = open_image(file, "wb");

        uint32 entry_fidx = frame->function - module->e->functions;
        sgx_fwrite(&entry_fidx, sizeof(uint32), 1, fp);

        _dump_stack(exec_env, frame, fp, (i==1));
        sgx_fclose(fp);
    } while(frame = frame->prev_frame);

    ocall_fprintf_int("stack_count: ", i);

    // frame stackのサイズを保存
    SGX_FILE *fp = open_image("frame.img", "wb");

    sgx_key_128bit_t key = {0};

    // SGX_FILE *fp2 = sgx_fopen_auto_key("frame_key.dat", "wb");

    sgx_fclose(fp);

    // SGX_FILE *fp2 = open_image("frame_key.dat", "w");
    // sgx_fclose(fp2);

    errno = 0;
    int32_t result = sgx_fexport_auto_key("frame.img", &key);

    ocall_print_key(&key);
    ocall_save_key(&key);

    // if(result != 0) {
    //     ocall_fprintf_int("failed to export key: ", result);
    //     ocall_fprintf_int("errno: ", errno);
    // }else {
    //     ocall_fprintf_str("key exported", "");
    // }




    // sgx_fexport_auto_key("test_fexport", NULL);

    // ocall_print_key(key);
    // ocall_print("key printed\n");

    // SGX_FILE *fp2 = sgx_fopen_auto_key("frame_key.dat", "wb");
    // sgx_fwrite(&key, sizeof(sgx_key_128bit_t), 1, fp2);
    // sgx_ferror(fp2);
    // sgx_fclose(fp2);



    return 0;
}

int is_dirty(uint64 pagemap_entry) {
    return (pagemap_entry>>62&1) | (pagemap_entry>>63&1);
}

int is_soft_dirty(uint64 pagemap_entry) {
    return (pagemap_entry >> 55 & 1);
}

// 副作用あり
int get_pagemap(unsigned long memory_addr) {
    const int PAGE_SIZE = 4096;
    SGX_FILE *memory_fp = open_image("memory.img", "wb");
    int fd;
    // プロセスのpagemapを開く
    fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd == -1) {
        // perror("Error opening pagemap");
        return -1;
    }

    // pfnに対応するpagemapエントリを取得
    unsigned long pfn = memory_addr / PAGE_SIZE;
    off_t offset = sizeof(uint64) * pfn;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        // perror("Error seeking to pagemap entry");
        close(fd);
        return -1;
    }
    return fd;
}

// dirty pageの場合1を返す. not dirtyならば0. ただし、Linux以外は必ず1を返す
int check_soft_dirty(int fd, uint8* addr) {
#if BH_PLATFORM_LINUX == 1
    const int PAGEMAP_LENGTH = 8;
    const int PAGE_SIZE = 4096;
    uint64 pagemap_entry;
    unsigned long pfn = (unsigned long)addr / PAGE_SIZE;
    off_t offset = sizeof(uint64) * pfn;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        // perror("Error seeking to pagemap entry");
        close(fd);
        return -1;
    }

    if (read(fd, &pagemap_entry, PAGEMAP_LENGTH) != PAGEMAP_LENGTH) {
        perror("Error reading pagemap entry");
        close(fd);
        return -1;
    }

    // dirty pageのみdump
    // if (is_dirty(pagemap_entry)) {
    return is_soft_dirty(pagemap_entry);
#else
    // linux以外は確定で1を返す
    return 1;
#endif
}

int dump_dirty_memory(WASMMemoryInstance *memory) {
    const int PAGE_SIZE = 4096;
    SGX_FILE *memory_fp = open_image("memory.img", "wb");
    uint64 pagemap_entry;

#if BH_PLATFORM_LINUX == 1
    int fd = get_pagemap(memory->memory_data);
#else
    // check_soft_dirtyでfdを使っているのでダミー用. 
    // もっといい実装がありそう
    int fd = 0;
#endif

    uint8* memory_data = memory->memory_data;
    uint8* memory_data_end = memory->memory_data_end;
    int i = 0;
    for (uint8* addr = memory->memory_data; addr < memory_data_end; addr += PAGE_SIZE, ++i) {

        // dirty pageのみdump
        // if (is_dirty(pagemap_entry)) {
        if (check_soft_dirty(fd, addr)) {
            // printf("[%x, %x]: dirty page\n", i*PAGE_SIZE, (i+1)*PAGE_SIZE);
            uint32 offset = (uint64)addr - (uint64)memory_data;
            // printf("i: %d\n", offset);
            sgx_fwrite(&offset, sizeof(uint32), 1, memory_fp);
            sgx_fwrite(addr, PAGE_SIZE, 1, memory_fp);
        }
    }

#if BH_PLATFORM_LINUX == 1
    close(fd);
#endif
    // fclose(memory_fp);
    sgx_fclose(memory_fp);
    return 0;
}


int wasm_dump_memory(WASMMemoryInstance *memory) {
    SGX_FILE *mem_size_fp = open_image("mem_page_count.img", "wb");

    dump_dirty_memory(memory);


    // printf("page_count: %d\n", memory->cur_page_count);
    
  
    sgx_fwrite(&(memory->cur_page_count), sizeof(uint32), 1, mem_size_fp);

    sgx_fclose(mem_size_fp);

    // デバッグのために、すべてのメモリも保存
    // SGX_FILE *all_memory_fp = open_image("all_memory.img", "wb");
    // sgx_fwrite(memory->memory_data, sizeof(uint8),
    //        memory->num_bytes_per_page * memory->cur_page_count, all_memory_fp);
    // fclose(all_memory_fp);
    // sgx_fclose(all_memory_fp);
    return 0;
}

int wasm_dump_global(WASMModuleInstance *module, WASMGlobalInstance *globals, uint8* global_data) {
    SGX_FILE *fp;
    const char *file = "global.img";
    fp = sgx_fopen_auto_key(file, "wb");
    if (fp == NULL) {
        // fprintf(stderr, "failed to open %s\n", file);
        return -1;
    }

    // WASMMemoryInstance *memory = module->default_memory;
    uint8 *global_addr;
    for (int i = 0; i < module->e->global_count; i++) {
        switch (globals[i].type) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
                global_addr = get_global_addr_for_migration(global_data, (globals+i));
                sgx_fwrite(global_addr, sizeof(uint32), 1, fp);
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                global_addr = get_global_addr_for_migration(global_data, (globals+i));
                sgx_fwrite(global_addr, sizeof(uint64), 1, fp);
                break;
            default:
                printf("type error:B\n");
                break;
        }
    }

    sgx_fclose(fp);
    return 0;
}

int wasm_dump_program_counter(
    WASMModuleInstance *module,
    WASMFunctionInstance *func,
    uint8 *frame_ip
)
{
    SGX_FILE *fp;
    const char *file = "program_counter.img";
    fp = sgx_fopen_auto_key(file, "wb");
    if (fp == NULL) {
        // fprintf(stderr, "failed to open %s\n", file);
        return -1;
    }

    uint32 fidx, p_offset;
    fidx = func - module->e->functions;
    p_offset = frame_ip - wasm_get_func_code(func);

    dump_value(&fidx, sizeof(uint32), 1, fp);
    dump_value(&p_offset, sizeof(uint32), 1, fp);

    return 0;
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
        //  uint32 *frame_tsp,
         uint8 *frame_ip_end,
         uint8 *else_addr,
         uint8 *end_addr,
         uint8 *maddr,
         bool done_flag)
{
    int rc;
    struct timespec ts1, ts2;
    // dump linear memory
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    rc = wasm_dump_memory(memory);
    clock_gettime(CLOCK_MONOTONIC, &ts2);
    // fprintf(stderr, "memory, %lu\n", get_time(ts1, ts2));
    // ocall_fprintf_int("memory:", get_time(ts1, ts2));

    if (rc < 0) {
        LOG_ERROR("Failed to dump linear memory\n");
        return rc;
    }

    // dump globals
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    rc = wasm_dump_global(module, globals, global_data);
    clock_gettime(CLOCK_MONOTONIC, &ts2);
    // fprintf(stderr, "global, %lu\n", get_time(ts1, ts2));
    if (rc < 0) {
        LOG_ERROR("Failed to dump globals\n");
        return rc;
    }

    // dump program counter
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    rc = wasm_dump_program_counter(module, cur_func, frame_ip);
    clock_gettime(CLOCK_MONOTONIC, &ts2);
    // fprintf(stderr, "program counter, %lu\n", get_time(ts1, ts2));
    if (rc < 0) {
        LOG_ERROR("Failed to dump program_counter\n");
        return rc;
    }

    // dump stack
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    rc = wasm_dump_stack(exec_env, frame);
    clock_gettime(CLOCK_MONOTONIC, &ts2);
    // fprintf(stderr, "stack, %lu\n", get_time(ts1, ts2));
    if (rc < 0) {
        LOG_ERROR("Failed to dump frame\n");
        return rc;
    }

    LOG_VERBOSE("Success to dump img for wamr\n");
    return 0;
}

static bool sig_flag = false;

// void wasm_runtime_checkpoint() {
//     wasm_set_checkpoint(true);
// }

inline 
void wasm_set_checkpoint(bool f) {
    sig_flag = f;
}

inline 
bool wasm_get_checkpoint() {
    return sig_flag;
}