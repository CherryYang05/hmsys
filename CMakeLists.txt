cmake_minimum_required(VERSION 3.10)
project(hmsys VERSION 0.1.0 LANGUAGES C)

# 指定子目录
add_subdirectory(src)

include(CTest)
enable_testing()
