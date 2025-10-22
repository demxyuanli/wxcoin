
# Render preview module
add_library(RenderPreview STATIC
    RenderPreviewDialog.cpp
    PreviewCanvas.cpp
    LightManager.cpp
    AntiAliasingManager.cpp
    RenderingManager.cpp
    GlobalSettingsPanel.cpp
    LightingPanel.cpp
    AntiAliasingPanel.cpp
    RenderingModePanel.cpp
    BackgroundStylePanel.cpp
    ObjectSettingsPanel.cpp
    ObjectManager.cpp
    ConfigValidator.cpp
    UndoManager.cpp
    BackgroundManager.cpp
)

target_include_directories(RenderPreview PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(RenderPreview PRIVATE
    wx::core
    wx::base
    Coin::Coin
)
