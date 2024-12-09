set (MIGRATION_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_MIGRATION=1)

include_directories(${MIGRATION_DIR})

# include(FetchContent)
# FetchContent_Declare(
#     wasmig
#     GIT_REPOSITORY https://github.com/funera1/wasmig.git
#     # GIT_TAG HEAD
#     GIT_TAG  porting-rust 
# )

# # Rustライブラリのソースを取得
# FetchContent_MakeAvailable(wasmig)

# # Rustライブラリのビルド設定
# add_custom_target(build_wasmig ALL
#     COMMAND cargo build --release
#     WORKING_DIRECTORY ${wasmig_SOURCE_DIR}
#     COMMENT "Building Rust static library"
# )

# # Rustライブラリの出力ディレクトリ
# set(WASMIG_LIB "${wasmig_SOURCE_DIR}/target/release/wasmig.a")

# FetchContent_GetProperties(wasmig)
# if (NOT wasmig_POPULATED)
#     message ("-- Fetching wasmig ..")
#     FetchContent_Populate(wasmig)
#     include_directories("${wasmig_SOURCE_DIR}")
#     include_directories("${wasmig_SOURCE_DIR}/include")
#     include_directories("${wasmig_SOURCE_DIR}/src")
#     # add_subdirectory("${wasmig_SOURCE_DIR}")
#     # add_subdirectory("${wasmig_SOURCE_DIR})
#     file (GLOB_RECURSE c_source_wasmig ${wasmig_SOURCE_DIR}/src/*.c)
# endif()

file (GLOB source_all ${MIGRATION_DIR}/*.c)

# set (MIGRATION_SOURCE ${source_all} ${c_source_wasmig})
set (MIGRATION_SOURCE ${source_all})