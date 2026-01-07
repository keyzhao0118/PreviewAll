cmake_minimum_required(VERSION 3.20)

project(PreviewAllServer
	LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Network REQUIRED)
find_package(7zip CONFIG REQUIRED)
find_package(KF5SyntaxHighlighting REQUIRED)

add_executable(PreviewAllServer WIN32
	resources/resource.qrc
	main.cpp
	previewoptionpanel.cpp
	previewoptionpanel.h
	previewallapplication.cpp
	previewallapplication.h
	previewallregister.cpp
	previewallregister.h

	previewarchive/guiddef.cpp
	
	
	previewarchive/archiveparser.cpp
	previewarchive/archiveparser.h
	previewarchive/archiveopencallback.cpp
	previewarchive/archiveopencallback.h
	previewarchive/instreamwrapper.cpp
	previewarchive/instreamwrapper.h
	previewarchive/archivetree.cpp
	previewarchive/archivetree.h
	previewarchive/archivetreewidget.cpp
	previewarchive/archivetreewidget.h
	previewarchive/archivetreewidgetitem.cpp
	previewarchive/archivetreewidgetitem.h
	previewarchive/archivepreviewwidget.cpp
	previewarchive/archivepreviewwidget.h
)

target_link_libraries(PreviewAllServer
	PRIVATE
		Qt5::Widgets
		Qt5::Network
		KeyWidgets
		7zip::7zip
		KF5::SyntaxHighlighting
)

# 添加此命令以在构建后复制 DLL
add_custom_command(TARGET PreviewAllServer POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"$<TARGET_FILE_DIR:7zip::7zip>/../bin/7zip.dll" # 从 .lib 目录找到 .dll
	$<TARGET_FILE_DIR:PreviewAllServer>
	COMMENT "Copying 7z.dll to output directory"
)
