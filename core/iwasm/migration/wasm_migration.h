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
        ocall_fprintf_str("failed to open", file);
        ocall_fprintf_int("errno migration l48", errno);
        return NULL;
    }
    return fp;
}

static SGX_FILE* open_type_stack(const char* file, const char* flag) {


    int src_fd;
    ssize_t bytesRead, bytesWritten;
    ssize_t buffer[4096];

    // ソースファイルを読み取り専用で開く
    src_fd = open(file, O_RDONLY);
    if (src_fd < 0) {
        ocall_print("failed to open file\n");
    }

    SGX_FILE *fp = sgx_fopen_auto_key("copy_file", flag);

    // // デスティネーションファイルを書き込み専用で開く（存在しない場合は作成）
    // dest_fd = open("copy", O_WRONLY | O_CREAT | O_TRUNC);
    // if (dest_fd == -1) {
    //     close(src_fd);
    // }

    // ソースファイルからデスティネーションファイルへデータをコピー
    while ((bytesRead = read(src_fd, buffer, 4096)) > 0) {
        ocall_fprintf_int("", buffer);
        bytesWritten = sgx_fwrite(buffer, bytesRead, 1, fp);
        // bytesWritten = write(dest_fd, buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            // perror("Failed to write to destination file");
            // close(src_fd);
            // exit(EXIT_FAILURE);
            ocall_print("failed to write\n");
        }
    }

    close(src_fd);
    sgx_fclose(fp);

    // int fd = open(file, O_RDONLY);
    // if (fd < 0){
    //     ocall_print("\nfd open failed\n");
    // }

    // int size_r;
    // size_t buf[1024];

    // while(1){
    //     size_r = read(fd, buf, sizeof(buf));
    //     if (size_r == 0){
    //         break;
    //     }else if (size_r < 0){
    //         ocall_print("read failed\n");
    //         break;
    //     }
    // }

    // ocall_print("buf: ");
    // ocall_fprintf_int("bufsize: ", sizeof(buf));
    // ocall_print("\n");
    // close(fd);

    return fp;
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
