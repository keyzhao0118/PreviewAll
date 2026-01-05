# 添加子目录
cmake_minimum_required(VERSION 3.20)

project(PreviewAll
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(PreviewAll)
add_subdirectory(PreviewAllHandler)
add_subdirectory(KeyZip)
add_subdirectory(KeyWidgets)