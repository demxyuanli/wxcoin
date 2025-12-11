# Logger module - unified logging system
set(LOGGER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AsyncLogger.cpp
)

set(LOGGER_HEADERS
    ${CMAKE_SOURCE_DIR}/include/logger/Logger.h
    ${CMAKE_SOURCE_DIR}/include/logger/AsyncLogger.h
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

# Link log4cxx if available
if(log4cxx_FOUND)
    target_link_libraries(CADLogger PUBLIC log4cxx)
    target_compile_definitions(CADLogger PUBLIC USE_LOG4CXX)
elseif(LOG4CXX_FOUND)
    target_link_libraries(CADLogger PUBLIC ${LOG4CXX_LIBRARIES})
    target_include_directories(CADLogger PUBLIC ${LOG4CXX_INCLUDE_DIRS})
    target_compile_definitions(CADLogger PUBLIC USE_LOG4CXX)
endif()

# Set library export properties
set_target_properties(CADLogger PROPERTIES
    FOLDER "Logger"
    EXPORT_NAME Logger
) 