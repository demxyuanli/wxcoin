cmake_minimum_required(VERSION 3.20)
project(CADNav)

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

# 设置 wxWidgets 根目录（如果使用 vcpkg）
if(CMAKE_TOOLCHAIN_FILE AND NOT wxWidgets_ROOT_DIR)
    set(wxWidgets_ROOT_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows" CACHE STRING "wxWidgets root directory")
endif()

# 输出配置信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
if(DEFINED wxWidgets_ROOT_DIR)
    message(STATUS "wxWidgets root: ${wxWidgets_ROOT_DIR}")
endif()

# 查找依赖
find_package(wxWidgets CONFIG REQUIRED)
find_package(Coin CONFIG REQUIRED)
find_package(OpenCASCADE CONFIG REQUIRED)

# 添加源代码目录
add_subdirectory(src)

# Add test directory if it exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test)
    add_subdirectory(test)
endif()

# 主程序源文件
set(MAIN_SOURCES
    ${CMAKE_SOURCE_DIR}/src/MainApplication.cpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME} WIN32 ${MAIN_SOURCES})

# 设置头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)



# 链接所有模块库
target_link_libraries(${PROJECT_NAME} PRIVATE
    CADUI          # UI模块（包含所有其他模块的依赖）
    wx::core
    wx::base
    Coin::Coin
    TKBO TKDE TKBin TKCAF
)

# Windows 下启用 Unicode 支持
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CADNav)

# MSVC 特定设置
if(MSVC)
    # 强制在 Debug 模式下使用 /ZI 支持"编辑并继续"
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/ZI /Od>)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++17)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
        # 复制 config 目录及其所有子目录和文件到目标目录
    set(CONFIG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/config")
    set(TARGET_DIR $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    
    # 使用 copy_directory 命令复制整个目录
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CONFIG_SOURCE_DIR}"
                "${TARGET_DIR}/config"
        COMMENT "Copying config directory and all subdirectories to ${TARGET_DIR}/config"
    )
endif()

# 调试依赖路径
message(STATUS "Project include dir: ${CMAKE_SOURCE_DIR}/include")