# 添加子目录
cmake_minimum_required(VERSION 3.20)

project(PreviewAll
	VERSION 0.1
	DESCRIPTION "Top-level project for PreviewAll"
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(PreviewServer)
add_subdirectory(PreviewRegister)

add_subdirectory(KeyZip)
