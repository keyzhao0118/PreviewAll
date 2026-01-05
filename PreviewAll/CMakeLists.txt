cmake_minimum_required(VERSION 3.20)

project(PreviewAll
	LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(KF5SyntaxHighlighting REQUIRED)

add_executable(PreviewAll WIN32
	resources/resource.qrc
	main.cpp
	previewoptionpanel.h
	previewoptionpanel.cpp
)

target_link_libraries(PreviewAll
	PRIVATE
		Qt5::Widgets
		KeyWidgets
		KF5::SyntaxHighlighting
)
