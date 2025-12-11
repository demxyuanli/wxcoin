cmake_minimum_required(VERSION 3.20)
set(VcpkgEnableManifest OFF)
project(CADVisBird)

# === vcpkg manifest mode configuration ===
# Enable manifest mode for automatic dependency management
set(VCPKG_MANIFEST_MODE OFF CACHE BOOL "Enable vcpkg manifest mode" FORCE)
if(VCPKG_MANIFEST_MODE)
    message(STATUS "vcpkg manifest mode enabled for automatic dependency management")
endif()

# Reduce unnecessary CMake checks
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)

# Override any problematic environment variables
if(DEFINED ENV{CMAKE_POLICY_VERSION_MINIMUM})
    unset(ENV{CMAKE_POLICY_VERSION_MINIMUM})
endif()

# Force set CMake policy version minimum to avoid environment issues
set(CMAKE_POLICY_VERSION_MINIMUM 3.5.0 CACHE STRING "" FORCE)

# Set minimum CMake policy version
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
endif()

# Compilation speed optimization
if(MSVC)
    # Enable multi-processor compilation
    add_compile_options(/MP)
    
    # Disable minimal rebuild to speed up compilation
    string(REPLACE "/Gm" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Gm" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    
    # Use parallel linking
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /CGTHREADS:8")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /CGTHREADS:8")
    
    # Enable incremental linking in Debug
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL")
    
    # Fast PDB generation
    add_compile_options($<$<CONFIG:Debug>:/Zi>)
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    
    # Reduce string pooling overhead
    add_compile_options(/Zc:inline)
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

# Force using vcpkg packages by setting search paths
if(CMAKE_TOOLCHAIN_FILE)
    set(Coin_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/share/coin" CACHE PATH "Coin directory" FORCE)
    set(OpenCASCADE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/share/opencascade" CACHE PATH "OpenCASCADE directory" FORCE)
    # TBB uses global vcpkg installation
    if(DEFINED ENV{VCPKG_ROOT})
        set(TBB_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows/share/tbb" CACHE PATH "TBB directory" FORCE)
    endif()
endif()

# 查找依赖
find_package(wxWidgets CONFIG REQUIRED)
find_package(Coin CONFIG REQUIRED)

find_package(OpenCASCADE CONFIG REQUIRED)

# Find TBB for parallel processing
find_package(TBB CONFIG REQUIRED)

# Find log4cxx for logging
find_package(log4cxx CONFIG QUIET)
if(NOT log4cxx_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LOG4CXX QUIET liblog4cxx)
    endif()
endif()

# 添加源代码目录
add_subdirectory(src)

# 主程序源文件
set(MAIN_SOURCES
    ${CMAKE_SOURCE_DIR}/src/MainApplicationDocking.cpp
    ${CMAKE_SOURCE_DIR}/src/resources/cadnav_icon.cpp
)

# 创建可执行文件
if(WIN32)
    # Windows 资源文件
    set(RESOURCE_FILES
        ${CMAKE_SOURCE_DIR}/resources/app.rc
    )
    add_executable(${PROJECT_NAME} WIN32 ${MAIN_SOURCES} ${RESOURCE_FILES})
else()
    add_executable(${PROJECT_NAME} ${MAIN_SOURCES})
endif()

# 设置头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
)



# 链接所有模块库
target_link_libraries(${PROJECT_NAME} PRIVATE
    UIFrame        # UI Frame模块
    CADGeometry    # Geometry模块
    CADMod         # Mod模块 (选择和高亮功能)
    wx::core
    wx::base
    wx::webview
    Coin::Coin
    TKBO TKDE TKBin TKCAF
)

# Enable Unicode support on Windows
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    
    # Reduce unnecessary symbol exports
    add_definitions(-D_WINDOWS -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif()

# Viewport debugging option
option(DEBUG_VIEWPORT_LOGS "Enable detailed viewport debug logging" OFF)
if(DEBUG_VIEWPORT_LOGS)
    add_compile_definitions(DEBUG_VIEWPORT_LOGS)
    message(STATUS "Viewport debug logging enabled")
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CADVisBird)

# MSVC specific settings
if(MSVC)
    # Set MSBuild property globally for all targets to suppress vcpkg manifest warnings
    set(CMAKE_VS_GLOBAL_VcpkgEnableManifest true CACHE BOOL "Enable vcpkg manifest in MSBuild" FORCE)
    
    # Suppress vcpkg manifest warnings by setting property
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
    
    # Use /Zi instead of /ZI for faster compilation
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/Od>)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++17)

    # Windows 资源编译设置
    if(WIN32)
        # 确保资源编译器能够找到 wxWidgets 头文件和项目根目录
        set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /I\"${wxWidgets_INCLUDE_DIRS}\" /I\"${CMAKE_SOURCE_DIR}\"")
    endif()
    
    # 复制 config 目录（使用stamp文件避免触发重新配置）
    set(CONFIG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/config")
    set(CONFIG_STAMP "${CMAKE_BINARY_DIR}/config_copy.stamp")
    
    # 使用 OUTPUT 和 DEPENDS 来正确处理依赖，避免每次都触发
    add_custom_command(
        OUTPUT ${CONFIG_STAMP}
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CONFIG_SOURCE_DIR}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config"
        COMMAND ${CMAKE_COMMAND} -E touch ${CONFIG_STAMP}
        DEPENDS "${CONFIG_SOURCE_DIR}"
        COMMENT "Copying config directory (only when changed)"
        VERBATIM
    )
    
    # 创建一个目标来依赖这个stamp文件
    add_custom_target(ConfigCopy ALL DEPENDS ${CONFIG_STAMP})
    add_dependencies(${PROJECT_NAME} ConfigCopy)
endif()

# 调试依赖路径
message(STATUS "Project include dir: ${CMAKE_SOURCE_DIR}/include")