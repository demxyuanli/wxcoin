#cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="D:/repos/vcpkg/scripts/buildsystems/vcpkg.cmake" -DwxWidgets_ROOT_DIR="D:/repos/vcpkg/installed/x64-windows"

cmake_minimum_required(VERSION 3.20)
project(FreeCADNavigation)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 Vcpkg 工具链文件
set(CMAKE_TOOLCHAIN_FILE "D:/repos/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# 查找依赖
find_package(wxWidgets REQUIRED COMPONENTS core base gl aui propgrid)
find_package(Coin REQUIRED)

# 包含 wxWidgets
include(${wxWidgets_USE_FILE})

# 源文件
set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/Application.cpp
    ${CMAKE_SOURCE_DIR}/src/Canvas.cpp
    ${CMAKE_SOURCE_DIR}/src/PickingAidManager.cpp
    ${CMAKE_SOURCE_DIR}/src/CoordinateSystemRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/GeometryFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/NavigationController.cpp
    ${CMAKE_SOURCE_DIR}/src/InputManager.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneManager.cpp
    ${CMAKE_SOURCE_DIR}/src/CoordinateSystemRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/Command.cpp
    ${CMAKE_SOURCE_DIR}/src/Globals.cpp
    ${CMAKE_SOURCE_DIR}/src/NavigationCube.cpp
    ${CMAKE_SOURCE_DIR}/src/GeometryObject.cpp
    ${CMAKE_SOURCE_DIR}/src/MainFrame.cpp
    ${CMAKE_SOURCE_DIR}/src/MouseHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/NavigationStyle.cpp
    ${CMAKE_SOURCE_DIR}/src/ObjectTreePanel.cpp
    ${CMAKE_SOURCE_DIR}/src/PropertyPanel.cpp
    ${CMAKE_SOURCE_DIR}/src/CreateCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/Logger.cpp
    ${CMAKE_SOURCE_DIR}/src/PositionDialog.cpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# 设置头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
    Coin::Coin
)

# 添加明确的库路径
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    link_directories("D:/repos/vcpkg/installed/x64-windows/debug/lib")
else()
    link_directories("D:/repos/vcpkg/installed/x64-windows/lib")
endif()

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${wxWidgets_LIBRARIES}
    Coin::Coin
)

# Windows 下启用 Unicode 支持
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# MSVC 特定设置
if(MSVC)
    # 强制在 Debug 模式下使用 /ZI 支持"编辑并继续"
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/ZI /Od>)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++17)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()

# 调试依赖路径
message(STATUS "Project include dir: ${CMAKE_SOURCE_DIR}/include")