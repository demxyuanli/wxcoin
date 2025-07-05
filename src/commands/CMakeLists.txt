# Commands module - application commands and geometry factory
set(COMMANDS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CreateCommand.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GeometryFactory.cpp
)

set(COMMANDS_HEADERS
    ${CMAKE_SOURCE_DIR}/include/Command.h
    ${CMAKE_SOURCE_DIR}/include/CreateCommand.h
    ${CMAKE_SOURCE_DIR}/include/GeometryFactory.h
)

add_library(CADCommands STATIC ${COMMANDS_SOURCES} ${COMMANDS_HEADERS})

target_include_directories(CADCommands PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
    ${OpenCASCADE_INCLUDE_DIR}
)

target_link_libraries(CADCommands PUBLIC
    CADCore
    CADOCC
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

set_target_properties(CADCommands PROPERTIES
    FOLDER "Commands"
    EXPORT_NAME Commands
) 