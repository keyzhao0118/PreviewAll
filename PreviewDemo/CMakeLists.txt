cmake_minimum_required(VERSION 3.20)

project(PreviewDemo
	LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(PreviewDemo WIN32
	main.cpp
	mainwindow.h
	mainwindow.cpp
	previewhandlerhost.h
	previewhandlerhost.cpp
)

target_link_libraries(PreviewDemo
	PRIVATE
		Qt5::Widgets
		Ole32
		Shlwapi
		Uuid
		Advapi32
)

target_compile_features(PreviewDemo PRIVATE cxx_std_17)


