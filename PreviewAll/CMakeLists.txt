cmake_minimum_required(VERSION 3.20)

project(PreviewAll
	LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets Network OpenGLWidgets Svg SvgWidgets LinguistTools REQUIRED)
find_package(7zip CONFIG REQUIRED)

add_executable(PreviewAll WIN32
	resources/resource.qrc
	resources/app.rc

	main.cpp
	previewallmenu.h
	previewallmenu.cpp

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

	previewimage/imageviewerwidget.cpp
	previewimage/imageviewerwidget.h
	previewimage/imageviewportwidget.cpp
	previewimage/imageviewportwidget.h
	previewimage/imageviewstatusbar.cpp
	previewimage/imageviewstatusbar.h

)

target_link_libraries(PreviewAll
	PRIVATE
		Qt6::Widgets
		Qt6::Network
		Qt6::OpenGLWidgets
		Qt6::Svg
		Qt6::SvgWidgets
		7zip::7zip
)

# 添加此命令以在构建后复制 DLL
add_custom_command(TARGET PreviewAll POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"$<TARGET_FILE_DIR:7zip::7zip>/../bin/7zip.dll" # 从 .lib 目录找到 .dll
	$<TARGET_FILE_DIR:PreviewAll>
	COMMENT "Copying 7z.dll to output directory"
)

if(WIN32)
	if(NOT WINDEPLOYQT_EXECUTABLE)
		message(FATAL_ERROR "windeployqt was not found. Install the Qt6 tools package for the active vcpkg triplet.")
	endif()

	get_filename_component(WINDEPLOYQT_DIR "${WINDEPLOYQT_EXECUTABLE}" DIRECTORY)
	set(WINDEPLOYQT_DEBUG_EXECUTABLE "${WINDEPLOYQT_DIR}/windeployqt.debug.bat")

	add_custom_command(TARGET PreviewAll POST_BUILD
		COMMAND "$<IF:$<CONFIG:Debug>,${WINDEPLOYQT_DEBUG_EXECUTABLE},${WINDEPLOYQT_EXECUTABLE}>"
			--dir "$<TARGET_FILE_DIR:PreviewAll>"
			--no-translations
			--compiler-runtime
			"$<TARGET_FILE:PreviewAll>"
		COMMENT "Deploying Qt runtime and plugins with windeployqt"
		COMMAND_EXPAND_LISTS
		VERBATIM
	)
endif()

# 国际化：lupdate 扫描源码更新 .ts，lrelease 编译生成 .qm
set(TS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/translations)
set(QM_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/translations)

add_custom_target(PreviewAll_translations ALL
	COMMAND Qt6::lupdate -extensions cpp,h -no-obsolete ${CMAKE_CURRENT_SOURCE_DIR} -ts
		${TS_DIR}/previewall_zh_CN.ts
		${TS_DIR}/previewall_en_US.ts
	COMMAND ${CMAKE_COMMAND} -E make_directory ${QM_DIR}
	COMMAND Qt6::lrelease ${TS_DIR}/previewall_zh_CN.ts -qm ${QM_DIR}/previewall_zh_CN.qm
	COMMAND Qt6::lrelease ${TS_DIR}/previewall_en_US.ts -qm ${QM_DIR}/previewall_en_US.qm
	COMMENT "Updating translations and generating .qm files..."
)

add_dependencies(PreviewAll PreviewAll_translations)
