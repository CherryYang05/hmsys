cmake_minimum_required(VERSION 3.10)
project(hmsys VERSION 0.1.0 LANGUAGES C)

# 设置可执行文件
add_executable(my_project main.c)

include(CTest)
enable_testing()
