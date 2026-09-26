cmake_minimum_required(VERSION 3.20)

project(PreviewAll
	LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets Network OpenGLWidgets Svg LinguistTools REQUIRED)
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
	previewtitlebar.cpp
	previewtitlebar.h
	previewtitlebarstyle.h
	previewloadpool.h

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

	previewmd/markdownpreviewwidget.cpp
	previewmd/markdownpreviewwidget.h

)

target_include_directories(PreviewAll
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(PreviewAll
	PRIVATE
		Qt6::Widgets
		Qt6::Network
		Qt6::OpenGLWidgets
		Qt6::Svg
		7zip::7zip
		Shell32
)

# 添加此命令以在构建后复制 DLL
add_custom_command(TARGET PreviewAll POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"$<TARGET_FILE:7zip::7zip>"
	$<TARGET_FILE_DIR:PreviewAll>
	COMMENT "Copying 7zip.dll to output directory"
	VERBATIM
)

if(WIN32)
	find_program(WINDEPLOYQT_EXECUTABLE
		NAMES windeployqt6 windeployqt
		HINTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/Qt6/bin"
		REQUIRED
	)

	get_filename_component(WINDEPLOYQT_DIR "${WINDEPLOYQT_EXECUTABLE}" DIRECTORY)
	set(WINDEPLOYQT_DEBUG_EXECUTABLE "${WINDEPLOYQT_DIR}/windeployqt.debug.bat")
	if(NOT EXISTS "${WINDEPLOYQT_DEBUG_EXECUTABLE}")
		message(FATAL_ERROR "The vcpkg Debug deployment wrapper is missing: ${WINDEPLOYQT_DEBUG_EXECUTABLE}")
	endif()

	add_custom_command(TARGET PreviewAll POST_BUILD
		COMMAND "$<IF:$<CONFIG:Debug>,${WINDEPLOYQT_DEBUG_EXECUTABLE},${WINDEPLOYQT_EXECUTABLE}>"
			--dir "$<TARGET_FILE_DIR:PreviewAll>"
			--compiler-runtime
			"$<IF:$<CONFIG:Debug>,--debug,--release>"
			"$<TARGET_FILE:PreviewAll>"
		COMMENT "Deploying Qt runtime and plugins with windeployqt"
		COMMAND_EXPAND_LISTS
		VERBATIM
	)

	# windeployqt deploys Qt image plugins but doesn't copy their third-party
	# vcpkg DLL dependencies (for example jpeg62.dll, tiff.dll and libwebp*.dll).
	# Keep the non-Qt runtime DLLs beside the app so those plugins load without
	# relying on the developer machine's PATH.
	set(VCPKG_RUNTIME_BIN "$<IF:$<CONFIG:Debug>,${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin,${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin>")
	add_custom_command(TARGET PreviewAll POST_BUILD
		COMMAND "${CMAKE_COMMAND}"
			"-DVCPKG_RUNTIME_BIN=${VCPKG_RUNTIME_BIN}"
			"-DOUTPUT_DIRECTORY=$<TARGET_FILE_DIR:PreviewAll>"
			-P "${CMAKE_CURRENT_LIST_DIR}/../cmake/CopyVcpkgRuntime.cmake"
		COMMENT "Deploying non-Qt vcpkg runtime DLLs"
		VERBATIM
	)
endif()

# Translation source updates are explicit; normal builds never modify tracked .ts files.
set(TS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/translations)
set(QM_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/translations)

add_custom_target(PreviewAll_update_translations
	COMMAND Qt6::lupdate -extensions cpp,h -no-obsolete ${CMAKE_CURRENT_SOURCE_DIR} -ts
		${TS_DIR}/previewall_zh_CN.ts
		${TS_DIR}/previewall_en_US.ts
	COMMENT "Updating translation sources"
	VERBATIM
)

set(QM_FILES)
foreach(LANGUAGE zh_CN en_US)
	set(QM_FILE "${QM_DIR}/previewall_${LANGUAGE}.qm")
	add_custom_command(OUTPUT "${QM_FILE}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${QM_DIR}"
		COMMAND Qt6::lrelease "${TS_DIR}/previewall_${LANGUAGE}.ts" -qm "${QM_FILE}"
		DEPENDS "${TS_DIR}/previewall_${LANGUAGE}.ts"
		VERBATIM
	)
	list(APPEND QM_FILES "${QM_FILE}")
endforeach()
add_custom_target(PreviewAll_translations ALL DEPENDS ${QM_FILES})

add_dependencies(PreviewAll PreviewAll_translations)

# Explicit verification target, sharing the application's deployed runtime.
add_executable(PreviewAllSmoke EXCLUDE_FROM_ALL
	../test/runtime-smoke.cpp
	resources/resource.qrc
	previewtitlebar.cpp
	previewmd/markdownpreviewwidget.cpp
	previewmd/markdownpreviewwidget.h
	previewarchive/archivepreviewwidget.cpp
	previewarchive/archivepreviewwidget.h
	previewarchive/archivetreewidget.cpp
	previewarchive/archivetreewidget.h
	previewarchive/archivetreewidgetitem.cpp
	previewarchive/archivetreewidgetitem.h
	previewarchive/archiveparser.cpp
	previewarchive/archiveopencallback.cpp
	previewarchive/archivetree.cpp
	previewarchive/instreamwrapper.cpp
	previewarchive/guiddef.cpp
)
target_include_directories(PreviewAllSmoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PreviewAllSmoke PRIVATE Qt6::Widgets 7zip::7zip)
add_dependencies(PreviewAllSmoke PreviewAll PreviewAllHandler)
