#ifndef _WASM_MIGRATION_H
#define _WASM_MIGRATION_H


// #include "../common/wasm_exec_env.h"
#include "../interpreter/wasm_interp.h"

#include "/opt/intel/sgxsdk/include/sgx_tprotected_fs.h"
#include "sgx_tprotected_fs.h"

int64_t get_time(struct timespec ts1, struct timespec ts2);

static inline uint8 *
get_global_addr_for_migration(uint8 *global_data, const WASMGlobalInstance *global)
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

static uint32
get_addr_offset(void* target, void* base)
{
    if (target == NULL) return -1;
    else return target - base;
}

static void*
set_addr_offset(void* base, uint32 offset)
{
    if (offset == -1) return NULL;
    else return base + offset;
}

static SGX_FILE* open_image(const char* file, const char* flag) {
    SGX_FILE *fp = sgx_fopen_auto_key(file, flag);

    // errno = 0;
    // const sgx_key_128bit_t key = {0x7b, 0x72, 0xfb, 0x7d, 0xda, 0xdb, 0x89, 0x3e, 0x28, 0x97, 0x45, 0xa7, 0x76, 0xf4, 0x51, 0xba};
    // SGX_FILE *fp = sgx_fopen(file, flag, &key);

    if (fp == NULL) {
        ocall_fprintf_str("open_image, failed to open", file);
        // ocall_printf_int("errno migration l48", errno);
        return NULL;
    }
    return fp;
}


static void convert_binary_to_sgx_file(const char* file_name, const char* sgx_file_name) {
    uint8_t *file_buf;
    uint32_t file_size;

    ocall_get_file_size(file_name, &file_size);


    ocall_printf_size("file_size", file_size);
   
    file_buf = (uint8_t *)malloc(sizeof(uint8_t) * file_size);
    if(file_buf == NULL) {
        ocall_fprintf_str("convert_binary_to_sgx_file, failed to allocate memory", "");
        return;
    }

    ocall_print("-----------\n");

    ocall_read_file_to_buffer(file_name, file_size, file_buf);


    // ocall_write_buffer_to_file(sgx_file_name, file_buf, file_size);


    SGX_FILE *fp = sgx_fopen_auto_key(sgx_file_name, "wb");
    sgx_fwrite(file_buf, file_size, 1, fp);
    sgx_fclose(fp);



}
 


// int wasm_dump(WASMExecEnv *exec_env,
//          WASMModuleInstance *module,
//          WASMMemoryInstance *memory,
//          WASMGlobalInstance *globals,
//          uint8 *global_data,
//          uint8 *global_addr,
//          WASMFunctionInstance *cur_func,
//          struct WASMInterpFrame *frame,
//          register uint8 *frame_ip,
//          register uint32 *frame_sp,
//          WASMBranchBlock *frame_csp,
//          uint8 *frame_ip_end,
//          uint8 *else_addr,
//          uint8 *end_addr,
//          uint8 *maddr,
//          bool done_flag);

// int wasm_restore(WASMModuleInstance *module,
//             WASMExecEnv *exec_env,
//             WASMFunctionInstance *cur_func,
//             WASMInterpFrame *prev_frame,
//             WASMMemoryInstance *memory,
//             WASMGlobalInstance *globals,
//             uint8 *global_data,
//             uint8 *global_addr,
//             WASMInterpFrame *frame,
//             register uint8 *frame_ip,
//             register uint32 *frame_lp,
//             register uint32 *frame_sp,
//             WASMBranchBlock *frame_csp,
//             uint8 *frame_ip_end,
//             uint8 *else_addr,
//             uint8 *end_addr,
//             uint8 *maddr,
//             bool *done_flag);
#endif // _WASM_MIGRATION_H
