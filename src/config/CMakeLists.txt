# Config module - configuration management and theme system
set(CONFIG_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThemeManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SvgIconManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ConstantsConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Coin3DConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LoggerConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderingConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EdgeSettingsConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LightingConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FontManager.cpp
)

set(CONFIG_HEADERS
    ${CMAKE_SOURCE_DIR}/include/config/ConfigManager.h
    ${CMAKE_SOURCE_DIR}/include/config/ThemeManager.h
    ${CMAKE_SOURCE_DIR}/include/config/SvgIconManager.h
    ${CMAKE_SOURCE_DIR}/include/config/ConstantsConfig.h
    ${CMAKE_SOURCE_DIR}/include/config/Coin3DConfig.h
    ${CMAKE_SOURCE_DIR}/include/config/LoggerConfig.h
    ${CMAKE_SOURCE_DIR}/include/config/RenderingConfig.h
    ${CMAKE_SOURCE_DIR}/include/config/EdgeSettingsConfig.h
    ${CMAKE_SOURCE_DIR}/include/config/LightingConfig.h
    ${CMAKE_SOURCE_DIR}/include/config/ThemeUIInterface.h
    ${CMAKE_SOURCE_DIR}/include/config/FontManager.h
)

# Create config static library
add_library(CADConfig STATIC ${CONFIG_SOURCES} ${CONFIG_HEADERS})

# Set header directories
target_include_directories(CADConfig PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(CADConfig PUBLIC
    CADCore
    CADLogger
    ${wxWidgets_LIBRARIES}
    Coin::Coin
)

# Set library export properties
set_target_properties(CADConfig PROPERTIES
    FOLDER "Config"
    EXPORT_NAME Config
) 