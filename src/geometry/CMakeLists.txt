# Geometry module CMakeLists.txt
# Handles all geometry file format readers

# Set source files
set(GEOMETRY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/GeometryReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/STEPReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/STEPImportOptimizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IGESReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OBJReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/STLReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BREPReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XTReader.cpp
)

# Set header files
set(GEOMETRY_HEADERS
    ${CMAKE_SOURCE_DIR}/include/GeometryReader.h
    ${CMAKE_SOURCE_DIR}/include/STEPReader.h
    ${CMAKE_SOURCE_DIR}/include/STEPImportOptimizer.h
    ${CMAKE_SOURCE_DIR}/include/IGESReader.h
    ${CMAKE_SOURCE_DIR}/include/OBJReader.h
    ${CMAKE_SOURCE_DIR}/include/STLReader.h
    ${CMAKE_SOURCE_DIR}/include/BREPReader.h
    ${CMAKE_SOURCE_DIR}/include/XTReader.h
)

# Create geometry library
add_library(CADGeometry STATIC ${GEOMETRY_SOURCES} ${GEOMETRY_HEADERS})

# Set target properties
set_target_properties(CADGeometry PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(CADGeometry PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(CADGeometry PUBLIC
    CADCore
    CADLogger
    CADOCC
    ${OpenCASCADE_LIBRARIES}
)

# Compiler definitions
target_compile_definitions(CADGeometry PRIVATE
    CADGEOMETRY_EXPORTS
)

# MSVC specific settings
if(MSVC)
    target_compile_options(CADGeometry PRIVATE /std:c++17)
endif()

# Set module folder for IDE organization
set_target_properties(CADGeometry PROPERTIES FOLDER "Modules")
