# Input module - input management and event handling
set(INPUT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/InputManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EventCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MouseHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DefaultInputState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PickingInputState.cpp
)

set(INPUT_HEADERS
    ${CMAKE_SOURCE_DIR}/include/InputManager.h
    ${CMAKE_SOURCE_DIR}/include/EventCoordinator.h
    ${CMAKE_SOURCE_DIR}/include/MouseHandler.h
    ${CMAKE_SOURCE_DIR}/include/DefaultInputState.h
    ${CMAKE_SOURCE_DIR}/include/PickingInputState.h
    ${CMAKE_SOURCE_DIR}/include/InputState.h
)

# Create input static library
add_library(CADInput STATIC ${INPUT_SOURCES} ${INPUT_HEADERS})

# Set header directories
target_include_directories(CADInput PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(CADInput PUBLIC
    CADCore
    ${wxWidgets_LIBRARIES}
    Coin::Coin
)

# Set library export properties
set_target_properties(CADInput PROPERTIES
    FOLDER "Input"
    EXPORT_NAME Input
) 