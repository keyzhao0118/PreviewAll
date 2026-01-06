cmake_minimum_required(VERSION 3.20)

project(PreviewAllHandler
	LANGUAGES CXX
)

find_package(Qt5 COMPONENTS Network REQUIRED)

add_library(PreviewAllHandler SHARED
	export.def
	dllmain.cpp
	previewallhandler.cpp
	previewallhandler.h
	previewallrequester.cpp
	previewallrequester.h
)

target_link_libraries(PreviewAllHandler 
	PRIVATE
		Qt5::Network
		Shlwapi
)
