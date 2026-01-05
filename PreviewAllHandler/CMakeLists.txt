cmake_minimum_required(VERSION 3.20)

project(PreviewAllHandler
	LANGUAGES CXX
)


find_package(Qt5 COMPONENTS Core REQUIRED)

add_library(PreviewAllHandler SHARED

)

target_link_libraries(PreviewAllHandler 
	PRIVATE
		Qt5::Core
)
