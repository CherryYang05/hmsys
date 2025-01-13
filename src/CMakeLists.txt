cmake_minimum_required(VERSION 3.10)
project(hmsys VERSION 0.1.0 LANGUAGES C)

# 定义可执行目标
add_executable(hmsys hmsys.c)

# 自动添加源文件(GLOB_RECURSE 递归查找所有源文件)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.c)
target_sources(hmsys PUBLIC ${sources})

# 链接 toml 库
target_link_libraries(hmsys PRIVATE toml)

# 设置目标的头文件路径
target_include_directories(hmsys PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 为 hmsys 可执行文件设置输出路径
set_target_properties(hmsys PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 设置宏定义 CONFIG_PATH
target_compile_definitions(hmsys PRIVATE CONFIG_PATH="${CMAKE_SOURCE_DIR}/config.toml")
# 设置宏定义 TRACE_PATH
target_compile_definitions(hmsys PRIVATE TRACE_PATH="${CMAKE_SOURCE_DIR}/SSDonTape.trace")