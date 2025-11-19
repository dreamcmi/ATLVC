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
