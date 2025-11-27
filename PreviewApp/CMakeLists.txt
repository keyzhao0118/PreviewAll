cmake_minimum_required(VERSION 3.20)

project(PreviewApp
	LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(PreviewApp WIN32
	main.cpp
	previewapppanel.h
	previewapppanel.cpp
)

target_link_libraries(PreviewApp
	PRIVATE
		Qt5::Widgets
		KeyWidgets
)
