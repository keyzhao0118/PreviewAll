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
	guiddef.cpp	
	main.cpp
	keyzipwindow.h
	keyzipwindow.cpp

	commonhelper.h
	commonhelper.cpp

	archiveparser.h
	archiveparser.cpp
	archiveextractor.h
	archiveextractor.cpp

	archivetreewidget.h
	archivetreewidget.cpp
	archivetreewidgetitem.h
	archivetreewidgetitem.cpp
	archivetree.h
	archivetree.cpp
	
	instreamwrapper.h
	instreamwrapper.cpp
	outstreamwrapper.h
	outstreamwrapper.cpp
	archiveopencallback.h
	archiveopencallback.cpp
	archiveextractcallback.cpp
	archiveextractcallback.h
)

target_link_libraries(KeyZip
	PRIVATE
		Qt5::Widgets
		KeyWidgets
		7zip::7zip
)

# 添加此命令以在构建后复制 DLL
add_custom_command(TARGET KeyZip POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"$<TARGET_FILE_DIR:7zip::7zip>/../bin/7zip.dll" # 从 .lib 目录找到 .dll
	$<TARGET_FILE_DIR:KeyZip>
	COMMENT "Copying 7z.dll to output directory"
)