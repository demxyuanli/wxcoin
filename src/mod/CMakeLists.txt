# Mod module - face selection and highlighting functionality
set(MOD_SOURCES
    FaceHighlightManager.cpp
    FaceInfoOverlay.cpp
    FaceQueryDialog.cpp
    Selection.cpp
    SoFCUnifiedSelection.cpp
    SoHighlightElementAction.cpp
    SoSelectionElementAction.cpp
    ViewProvider.cpp
)

set(MOD_HEADERS
    ${CMAKE_SOURCE_DIR}/include/mod/FaceHighlightManager.h
    ${CMAKE_SOURCE_DIR}/include/mod/FaceInfoOverlay.h
    ${CMAKE_SOURCE_DIR}/include/mod/FaceQueryDialog.h
    ${CMAKE_SOURCE_DIR}/include/mod/Selection.h
    ${CMAKE_SOURCE_DIR}/include/mod/SoFCUnifiedSelection.h
    ${CMAKE_SOURCE_DIR}/include/mod/SoHighlightElementAction.h
    ${CMAKE_SOURCE_DIR}/include/mod/SoSelectionElementAction.h
    ${CMAKE_SOURCE_DIR}/include/mod/ViewProvider.h
)

# Create mod static library
add_library(CADMod STATIC ${MOD_SOURCES} ${MOD_HEADERS})

target_include_directories(CADMod PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
    ${OpenCASCADE_INCLUDE_DIR}
)

target_link_libraries(CADMod PUBLIC
    CADCore
    CADLogger
    CADConfig
    CADOCC
    CADGeometry
    CADRenderingToolkit
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

# Enable precompiled headers for faster compilation
if(MSVC)
    target_precompile_headers(CADMod PRIVATE
        <wx/wx.h>
        <vector>
        <string>
        <memory>
        <Inventor/nodes/SoSeparator.h>
    )
endif()

set_target_properties(CADMod PROPERTIES
    FOLDER "Modules"
    EXPORT_NAME Mod
)

