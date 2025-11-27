cmake_minimum_required(VERSION 3.20)

project(KeyWidgets
	LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_library(KeyWidgets STATIC
	
	include/keyslideswitch.h
	include/keycardwidget.h
	
	src/keyslideswitch.cpp
	src/keycardwidget.cpp
)

target_include_directories(KeyWidgets
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(KeyWidgets
	PUBLIC
		Qt5::Widgets
)