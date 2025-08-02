
# 渲染预览模块
add_library(RenderPreview STATIC
    RenderPreviewDialog.cpp
    PreviewCanvas.cpp
    GlobalSettingsPanel.cpp
    ObjectSettingsPanel.cpp
)

target_include_directories(RenderPreview PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(RenderPreview PRIVATE
    wx::core
    wx::base
    Coin::Coin
)
