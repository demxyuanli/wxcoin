cmake_minimum_required(VERSION 3.20)

set(ASYNC_ENGINE_SOURCES
    AsyncComputeEngine.cpp
    GeometryComputeTasks.cpp
    AsyncEngineIntegration.cpp
)

set(ASYNC_ENGINE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/async/AsyncComputeEngine.h
    ${CMAKE_SOURCE_DIR}/include/async/GeometryComputeTasks.h
    ${CMAKE_SOURCE_DIR}/include/async/AsyncEngineIntegration.h
)

add_library(AsyncEngine STATIC
    ${ASYNC_ENGINE_SOURCES}
    ${ASYNC_ENGINE_HEADERS}
)

target_include_directories(AsyncEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
)

target_link_libraries(AsyncEngine PUBLIC
    ${wxWidgets_LIBRARIES}
    TBB::tbb
    TKernel
    TKMath
    TKBRep
    TKGeomBase
    TKGeomAlgo
    TKTopAlgo
    TKMesh
    CADLogger
    CADOCC
)

set_target_properties(AsyncEngine PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    FOLDER "Libraries"
)

if(MSVC)
    target_compile_options(AsyncEngine PRIVATE /W4)
else()
    target_compile_options(AsyncEngine PRIVATE -Wall -Wextra)
endif()

