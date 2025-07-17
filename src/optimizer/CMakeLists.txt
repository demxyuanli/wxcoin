set(OPTIMIZER_SOURCES
    PerformanceOptimizer.cpp
)

set(OPTIMIZER_TEST_SOURCES
    PerformanceTest.cpp
)

set(OPTIMIZER_HEADERS
    ${CMAKE_SOURCE_DIR}/include/optimizer/PerformanceOptimizer.h
)

# Create optimizer library
add_library(optimizer STATIC ${OPTIMIZER_SOURCES} ${OPTIMIZER_HEADERS})

# Set include directories
target_include_directories(optimizer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(optimizer
    CADCore
    CADLogger
    CADCommands
    CADOCC
    ${OpenCASCADE_LIBRARIES}
    ${wxWidgets_LIBRARIES}
)

# Set compiler flags
target_compile_features(optimizer PRIVATE cxx_std_17)

# Set compiler-specific flags
if(MSVC)
    target_compile_options(optimizer PRIVATE /W4)
else()
    target_compile_options(optimizer PRIVATE -Wall -Wextra -O2)
endif()

# Add to parent project
set(OPTIMIZER_LIBRARY optimizer PARENT_SCOPE)

