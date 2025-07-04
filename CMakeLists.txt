cmake_minimum_required(VERSION 3.20)
project(FreeCADNavigation)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
endif()

# 检查是否通过预设或环境变量设置了工具链文件
if(NOT CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

# 输出配置信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
if(DEFINED wxWidgets_ROOT_DIR)
    message(STATUS "wxWidgets root: ${wxWidgets_ROOT_DIR}")
endif()

# 查找依赖
find_package(wxWidgets REQUIRED COMPONENTS core base gl aui propgrid)
find_package(Coin REQUIRED)

# 包含 wxWidgets
include(${wxWidgets_USE_FILE})

# 源文件
set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/Application.cpp
    ${CMAKE_SOURCE_DIR}/src/Canvas.cpp
    ${CMAKE_SOURCE_DIR}/src/RenderingEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/ViewportManager.cpp
    ${CMAKE_SOURCE_DIR}/src/EventCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/NavigationCubeManager.cpp
    ${CMAKE_SOURCE_DIR}/src/PickingAidManager.cpp
    ${CMAKE_SOURCE_DIR}/src/CoordinateSystemRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/GeometryFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/NavigationController.cpp
    ${CMAKE_SOURCE_DIR}/src/InputManager.cpp
    ${CMAKE_SOURCE_DIR}/src/DefaultInputState.cpp
    ${CMAKE_SOURCE_DIR}/src/PickingInputState.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneManager.cpp
    ${CMAKE_SOURCE_DIR}/src/CoordinateSystemRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/Command.cpp
    ${CMAKE_SOURCE_DIR}/src/NavigationCube.cpp
    ${CMAKE_SOURCE_DIR}/src/GeometryObject.cpp
    ${CMAKE_SOURCE_DIR}/src/MainFrame.cpp
    ${CMAKE_SOURCE_DIR}/src/NavigationCubeConfigDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/MouseHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/NavigationStyle.cpp
    ${CMAKE_SOURCE_DIR}/src/ObjectTreePanel.cpp
    ${CMAKE_SOURCE_DIR}/src/PropertyPanel.cpp
    ${CMAKE_SOURCE_DIR}/src/CreateCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/Logger.cpp
    ${CMAKE_SOURCE_DIR}/src/PositionDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/DPIManager.cpp
    ${CMAKE_SOURCE_DIR}/src/DPIAwareRendering.cpp
    ${CMAKE_SOURCE_DIR}/src/CuteNavCube.cpp
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

# 添加明确的库路径（如果使用vcpkg）
if(CMAKE_TOOLCHAIN_FILE AND wxWidgets_ROOT_DIR)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        link_directories("${wxWidgets_ROOT_DIR}/debug/lib")
    else()
        link_directories("${wxWidgets_ROOT_DIR}/lib")
    endif()
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