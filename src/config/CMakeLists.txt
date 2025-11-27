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
    ${CMAKE_CURRENT_SOURCE_DIR}/SelectionColorConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SelectionHighlightConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UnifiedConfigManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigManagerDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/ConfigCategoryEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/GeneralConfigEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/ThemeConfigEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/RenderingConfigEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/LightingConfigEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/TypographyConfigEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/LayoutConfigEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/DockLayoutConfigEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/LightingSettingsEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/EdgeSettingsEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/RenderPreviewEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/ConfigEditorFactory.cpp
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
    ${CMAKE_SOURCE_DIR}/include/config/SelectionColorConfig.h
    ${CMAKE_SOURCE_DIR}/include/config/SelectionHighlightConfig.h
    ${CMAKE_SOURCE_DIR}/include/config/UnifiedConfigManager.h
    ${CMAKE_SOURCE_DIR}/include/config/ConfigManagerDialog.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/ConfigCategoryEditor.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/GeneralConfigEditor.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/ThemeConfigEditor.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/RenderingConfigEditor.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/LightingConfigEditor.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/TypographyConfigEditor.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/LayoutConfigEditor.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/DockLayoutConfigEditor.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/LightingSettingsEditor.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/EdgeSettingsEditor.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/RenderPreviewEditor.h
    ${CMAKE_SOURCE_DIR}/include/config/editor/ConfigEditorFactory.h
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
    docking
    ${wxWidgets_LIBRARIES}
    Coin::Coin
)

# Set library export properties
set_target_properties(CADConfig PROPERTIES
    FOLDER "Config"
    EXPORT_NAME Config
) 