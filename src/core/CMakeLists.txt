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
# Note: Coin3D must be PUBLIC because GeometryObject.h includes Inventor headers
target_include_directories(CADCore PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${Coin3D_INCLUDE_DIRS}
)

# wxWidgets is PRIVATE - only Application.h needs it, and UI modules link wxWidgets directly
target_include_directories(CADCore PRIVATE
    ${wxWidgets_INCLUDE_DIRS}
)

# Link dependencies
# Coin must be PUBLIC because GeometryObject.h uses SoSeparator, SoTransform, etc.
target_link_libraries(CADCore PUBLIC
    Coin::Coin
)

# wxWidgets is PRIVATE - UI modules that use Application.h already link wxWidgets
target_link_libraries(CADCore PRIVATE
    ${wxWidgets_LIBRARIES}
)

# Set library export properties
set_target_properties(CADCore PROPERTIES
    FOLDER "Core"
    EXPORT_NAME Core
) 