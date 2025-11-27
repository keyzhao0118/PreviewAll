cmake_minimum_required(VERSION 3.20)
project(PreviewServer
	LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(KF5SyntaxHighlighting REQUIRED)

add_executable(PreviewServer WIN32
	main.cpp
)

target_link_libraries(PreviewServer 
	PRIVATE
		Qt5::Widgets
		KF5::SyntaxHighlighting
)
