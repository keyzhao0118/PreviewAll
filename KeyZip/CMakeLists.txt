cmake_minimum_required(VERSION 3.20)

project(KeyZip
	LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(7zip CONFIG REQUIRED)

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
	instreamwrapper.h
	instreamwrapper.cpp
	archiveopencallback.h
	archiveopencallback.cpp
	guiddef.cpp
)

target_link_libraries(KeyZip
	PRIVATE
		Qt5::Widgets
		7zip::7zip
)

# 添加此命令以在构建后复制 DLL
add_custom_command(TARGET KeyZip POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"$<TARGET_FILE_DIR:7zip::7zip>/../bin/7zip.dll" # 从 .lib 目录找到 .dll
	$<TARGET_FILE_DIR:KeyZip>
	COMMENT "Copying 7z.dll to output directory"
)