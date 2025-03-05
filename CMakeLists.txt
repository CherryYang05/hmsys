cmake_minimum_required(VERSION 3.10)
project(hmsys VERSION 0.1.0 LANGUAGES C)

# 设置库目录和头文件目录
set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)

# 添加 include 文件夹到头文件搜索路径
include_directories(${CMAKE_SOURCE_DIR}/include)

# 将 lib 目录添加到库搜索路径
link_directories(${LIB_DIR})

# 指定子目录
add_subdirectory(src)

# 指定可执行文件的输出路径
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
include(CTest)
enable_testing()

# 添加测试目标
add_test(NAME test_mapping COMMAND ${CMAKE_SOURCE_DIR}/bin/test_mapping)
add_test(NAME test_cache COMMAND ${CMAKE_SOURCE_DIR}/bin/test_cache)
# 将 config.toml 的路径作为编译选项传递给程序
# add_definitions(-DCONFIG_PATH="${CMAKE_SOURCE_DIR}/config.toml")
set(CMAKE_BUILD_TYPE Debug)
