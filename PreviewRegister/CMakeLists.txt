cmake_minimum_required(VERSION 3.20)

project(PreviewRegister)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(PreviewRegister WIN32
	main.cpp
	registermainpanel.h
	registermainpanel.cpp
	animatedswitch.h
	animatedswitch.cpp
)

target_link_libraries(PreviewRegister
	PRIVATE
		Qt5::Widgets
)
