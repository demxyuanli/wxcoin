# Core module - basic functionality and command system
set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/GeometryObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DPIManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DPIAwareRendering.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PerformanceBus.cpp
)

set(CORE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/Application.h
    ${CMAKE_SOURCE_DIR}/include/GeometryObject.h
    ${CMAKE_SOURCE_DIR}/include/DPIManager.h
    ${CMAKE_SOURCE_DIR}/include/DPIAwareRendering.h
    ${CMAKE_SOURCE_DIR}/include/utils/PerformanceBus.h
    ${CMAKE_SOURCE_DIR}/include/core/ThreadSafeCollector.h
)

# Create core static library
add_library(CADCore STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# Set header directories
target_include_directories(CADCore PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(CADCore PUBLIC
    ${wxWidgets_LIBRARIES}
    Coin::Coin
)

# Set library export properties
set_target_properties(CADCore PROPERTIES
    FOLDER "Core"
    EXPORT_NAME Core
) 