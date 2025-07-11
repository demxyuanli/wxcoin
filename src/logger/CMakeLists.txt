# Logger module - unified logging system
set(LOGGER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Logger.cpp
)

set(LOGGER_HEADERS
    ${CMAKE_SOURCE_DIR}/include/logger/Logger.h
)

# Create logger static library
add_library(CADLogger STATIC ${LOGGER_SOURCES} ${LOGGER_HEADERS})

# Set header directories
target_include_directories(CADLogger PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(CADLogger PUBLIC
    ${wxWidgets_LIBRARIES}
)

# Set library export properties
set_target_properties(CADLogger PROPERTIES
    FOLDER "Logger"
    EXPORT_NAME Logger
) 