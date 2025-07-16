# OpenCASCADE module - CAD geometry kernel and modeling
set(OPENCASCADE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCGeometry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCViewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCShapeBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCBrepConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCMeshConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/STEPReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GeometryFactory.cpp
)

set(OPENCASCADE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/OCCGeometry.h
    ${CMAKE_SOURCE_DIR}/include/OCCViewer.h
    ${CMAKE_SOURCE_DIR}/include/OCCShapeBuilder.h
    ${CMAKE_SOURCE_DIR}/include/OCCBrepConverter.h
    ${CMAKE_SOURCE_DIR}/include/OCCMeshConverter.h
    ${CMAKE_SOURCE_DIR}/include/STEPReader.h
    ${CMAKE_SOURCE_DIR}/include/GeometryFactory.h
)

# Create OpenCASCADE static library
add_library(CADOCC STATIC ${OPENCASCADE_SOURCES} ${OPENCASCADE_HEADERS})

# Set header directories
target_include_directories(CADOCC PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
    ${OpenCASCADE_INCLUDE_DIR}
)

# Link dependencies
target_link_libraries(CADOCC PUBLIC
    CADCore
    CADLogger
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

# Set library export properties
set_target_properties(CADOCC PROPERTIES
    FOLDER "OpenCASCADE"
    EXPORT_NAME OCC
) 