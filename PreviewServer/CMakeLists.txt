#cmake_minimum_required(VERSION 3.20)
project(MyQtApp)

# 启用自动生成 moc、rcc、uic
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 指定 C++ 标准
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt 包
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# 查找 KF5SyntaxHighlighting 包
find_package(KF5SyntaxHighlighting REQUIRED)

# 源文件
add_executable(MyQtApp WIN32
	main.cpp
)

# 链接 Qt 模块
target_link_libraries(MyQtApp 
	PRIVATE
		Qt5::Widgets
		KF5::SyntaxHighlighting
)
