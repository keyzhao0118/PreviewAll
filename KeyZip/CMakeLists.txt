cmake_minimum_required(VERSION 3.17)

project(KeyZip)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(LibArchive REQUIRED)

add_executable(KeyZip WIN32
	main.cpp
	archiveparser.h
	archiveparser.cpp
	commonhelper.h
	commonhelper.cpp
	archivetreewidget.h
	archivetreewidget.cpp
	keyzipwindow.h 
	keyzipwindow.cpp
)

target_link_libraries(KeyZip
	PRIVATE
		Qt5::Widgets
		LibArchive::LibArchive
)