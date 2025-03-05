cmake_minimum_required(VERSION 3.10)
project(hmsys VERSION 0.1.0 LANGUAGES C)

# 添加编译选项，忽略警告
add_compile_options(-w)

# 定义可执行目标
add_executable(hmsys hmsys.c)

# 自动添加源文件(GLOB_RECURSE 递归查找所有源文件)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.c)
# 排除测试文件
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/test_mapping.c)
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/test_cache.c)
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

# 设置宏定义 STATS_PATH
target_compile_definitions(hmsys PRIVATE STATS_PATH="${CMAKE_SOURCE_DIR}/stats.txt")

# 设置宏定义 RESPONSE_PATH
target_compile_definitions(hmsys PRIVATE RESPONSE_PATH="${CMAKE_SOURCE_DIR}/response.txt")

# 添加测试程序 - 地址映射测试
add_executable(test_mapping test_mapping.c cache_layer.c)
target_include_directories(test_mapping PRIVATE ${CMAKE_SOURCE_DIR}/include)
set_target_properties(test_mapping PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 添加测试程序 - 缓存测试
add_executable(test_cache test_cache.c cache_layer.c cache.c config.c)
target_include_directories(test_cache PRIVATE ${CMAKE_SOURCE_DIR}/include)
set_target_properties(test_cache PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
target_link_libraries(test_cache PRIVATE toml)
target_compile_definitions(test_cache PRIVATE CONFIG_PATH="${CMAKE_SOURCE_DIR}/config.toml")