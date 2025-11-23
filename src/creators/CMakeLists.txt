# Geometry Creators module - individual geometry creation classes

set(CREATORS_SOURCES
    GeometryCreator.cpp
    StandardCreators.cpp
    GeometryCreatorFactory.cpp
)

set(CREATORS_HEADERS
    ${CMAKE_SOURCE_DIR}/include/creators/IGeometryCreator.h
    ${CMAKE_SOURCE_DIR}/include/creators/GeometryCreator.h
    ${CMAKE_SOURCE_DIR}/include/creators/GeometryCreatorFactory.h
)

# Create geometry creators static library
add_library(CADCreators STATIC ${CREATORS_SOURCES} ${CREATORS_HEADERS})

target_include_directories(CADCreators PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
    ${OpenCASCADE_INCLUDE_DIR}
)

target_link_libraries(CADCreators PUBLIC
    CADCommands  # Depends on CADCommands for GeometryFactory
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

set_target_properties(CADCreators PROPERTIES
    FOLDER "Creators"
    EXPORT_NAME Creators
)
