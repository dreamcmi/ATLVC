cmake_minimum_required(VERSION 4.0)
project(ATLVC C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 针对 MSVC 的编译选项：禁用不必要的警告，避免干扰
if(MSVC)
    # 禁用编码警告（如果已设置 UTF-8 BOM，可禁用）
    add_compile_options(/wd4819)
    # 禁用“未使用参数”警告
    add_compile_options(/wd4100)
    # 启用 UTF-8 编码（MSVC 2019+ 支持）
    add_compile_options(/utf-8)
endif()

add_subdirectory(libatlvc)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} PRIVATE libatlvc)

if(MSVC)
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        # 移除 /RTC1（与 O2 冲突）
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        string(REPLACE "/RTC1" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

        # Release 模式：保留调试信息 + 优化
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /O2 /MD")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi /O2 /MD")

        # 链接器：启用调试信息，禁用 LTCG（避免符号丢失）
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
        string(REPLACE "/LTCG" "" CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

        # 配置 PDB 路径
        set_target_properties(${PROJECT_NAME} PROPERTIES
                PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
                PDB_NAME_RELEASE "ATLVC"
                PDB_GENERATED_ATLVC TRUE
        )
    endif ()
endif ()