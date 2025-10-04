# Param Module - Parameter Management System
# This module handles all parameter management, validation, and related functionality

set(PARAM_SOURCES
    MeshParameterManagerSimple.cpp
    MeshParameterValidator.cpp
    NormalValidator.cpp
)

set(PARAM_HEADERS
    ${CMAKE_SOURCE_DIR}/include/MeshParameterManagerSimple.h
    ${CMAKE_SOURCE_DIR}/include/MeshParameterValidator.h
    ${CMAKE_SOURCE_DIR}/include/NormalValidator.h
    ${CMAKE_SOURCE_DIR}/include/MeshParamNames.h
)

# Create the param library
add_library(CADParam STATIC ${PARAM_SOURCES})

# Set target properties
set_target_properties(CADParam PROPERTIES
    OUTPUT_NAME "CADParam"
    FOLDER "Libraries"
)

# Include directories
target_include_directories(CADParam PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/opencascade
    ${CMAKE_SOURCE_DIR}/include/rendering
    ${CMAKE_SOURCE_DIR}/include/logger
)

# Link dependencies
target_link_libraries(CADParam PUBLIC
    CADLogger
    CADOCC
    CADRendering
)

# Compiler definitions
target_compile_definitions(CADParam PRIVATE
    PARAM_MODULE_EXPORTS
)

# Compiler features
target_compile_features(CADParam PUBLIC cxx_std_17)

# Installation
install(TARGETS CADParam
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${PARAM_HEADERS}
    DESTINATION include
)
