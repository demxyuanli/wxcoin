# Unified Parameter Management System
# CMakeLists.txt for the unified parameter management system

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 源文件
set(UNIFIED_PARAM_SOURCES
    UnifiedParameterTree.cpp
    ParameterRegistry.cpp
    UpdateCoordinator.cpp
    UnifiedParameterIntegration.cpp
    UnifiedParameterExample.cpp
)

# 头文件
set(UNIFIED_PARAM_HEADERS
    ${CMAKE_SOURCE_DIR}/include/param/UnifiedParameterTree.h
    ${CMAKE_SOURCE_DIR}/include/param/ParameterRegistry.h
    ${CMAKE_SOURCE_DIR}/include/param/UpdateCoordinator.h
    ${CMAKE_SOURCE_DIR}/include/param/UnifiedParameterIntegration.h
)

# 创建静态库
add_library(UnifiedParameterSystem STATIC ${UNIFIED_PARAM_SOURCES} ${UNIFIED_PARAM_HEADERS})

# 链接必要的库
target_link_libraries(UnifiedParameterSystem
    ${OpenCASCADE_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${Inventor_LIBRARIES}
)

# 设置编译选项
target_compile_definitions(UnifiedParameterSystem PRIVATE
    -DUNIFIED_PARAMETER_SYSTEM_VERSION="1.0.0"
    -DUNIFIED_PARAMETER_SYSTEM_BUILD_DATE="${CMAKE_BUILD_DATE}"
)

# 设置包含目录
target_include_directories(UnifiedParameterSystem PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIRS}
    ${wxWidgets_INCLUDE_DIRS}
    ${Inventor_INCLUDE_DIRS}
)

# 创建示例可执行文件
add_executable(UnifiedParameterExample UnifiedParameterExample.cpp)
target_link_libraries(UnifiedParameterExample
    UnifiedParameterSystem
    ${OpenCASCADE_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${Inventor_LIBRARIES}
)

# 设置示例的包含目录
target_include_directories(UnifiedParameterExample PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIRS}
    ${wxWidgets_INCLUDE_DIRS}
    ${Inventor_INCLUDE_DIRS}
)

# 安装规则
install(TARGETS UnifiedParameterSystem
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${UNIFIED_PARAM_HEADERS}
    DESTINATION include/param
)

# 测试目标（可选）
if(BUILD_TESTS)
    enable_testing()
    
    # 创建测试可执行文件
    add_executable(UnifiedParameterTest
        UnifiedParameterTest.cpp
    )
    
    target_link_libraries(UnifiedParameterTest
        UnifiedParameterSystem
        ${OpenCASCADE_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${Inventor_LIBRARIES}
    )
    
    # 添加测试
    add_test(NAME UnifiedParameterSystemTest
        COMMAND UnifiedParameterTest
    )
endif()

# 打印构建信息
message(STATUS "Unified Parameter System:")
message(STATUS "  - Version: 1.0.0")
message(STATUS "  - Sources: ${UNIFIED_PARAM_SOURCES}")
message(STATUS "  - Headers: ${UNIFIED_PARAM_HEADERS}")
message(STATUS "  - Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  - C++ standard: ${CMAKE_CXX_STANDARD}")

# 设置输出目录
set_target_properties(UnifiedParameterSystem PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(UnifiedParameterExample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 添加编译选项
if(MSVC)
    target_compile_options(UnifiedParameterSystem PRIVATE /W4)
    target_compile_options(UnifiedParameterExample PRIVATE /W4)
else()
    target_compile_options(UnifiedParameterSystem PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(UnifiedParameterExample PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(UNIFIED_PARAM_SOURCES PRIVATE DEBUG_UNIFIED_PARAMETER_SYSTEM)
    message(STATUS "  - Debug mode enabled")
endif()

# 性能优化
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(UNIFIED_PARAM_SOURCES PRIVATE RELEASE_UNIFIED_PARAMETER_SYSTEM)
    message(STATUS "  - Release mode enabled")
endif()