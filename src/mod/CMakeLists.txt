# Mod module - face selection and highlighting functionality
set(MOD_SOURCES
    EventConverter.cpp
    FaceInfoOverlay.cpp
    FaceQueryDialog.cpp
    Selection.cpp
    SoFCSelection.cpp
    SoFCSelectionContext.cpp
    SoFCUnifiedSelection.cpp
    SoHighlightElementAction.cpp
    SoSelectionElementAction.cpp
    ViewProvider.cpp
)

set(MOD_HEADERS
    ${CMAKE_SOURCE_DIR}/include/mod/EventConverter.h
    ${CMAKE_SOURCE_DIR}/include/mod/FaceInfoOverlay.h
    ${CMAKE_SOURCE_DIR}/include/mod/FaceQueryDialog.h
    ${CMAKE_SOURCE_DIR}/include/mod/Selection.h
    ${CMAKE_SOURCE_DIR}/include/mod/SoFCSelection.h
    ${CMAKE_SOURCE_DIR}/include/mod/SoFCSelectionContext.h
    ${CMAKE_SOURCE_DIR}/include/mod/SoFCUnifiedSelection.h
    ${CMAKE_SOURCE_DIR}/include/mod/SoHighlightElementAction.h
    ${CMAKE_SOURCE_DIR}/include/mod/SoSelectionElementAction.h
    ${CMAKE_SOURCE_DIR}/include/mod/ViewProvider.h
)

set(MOD_SOURCES
    EventConverter.cpp
    FaceInfoOverlay.cpp
    FaceQueryDialog.cpp
    Selection.cpp
    SoFCSelection.cpp
    SoFCSelectionContext.cpp
    SoFCUnifiedSelection.cpp
    SoHighlightElementAction.cpp
    SoSelectionElementAction.cpp
    ViewProvider.cpp
)

# Create mod static library
add_library(CADMod STATIC ${MOD_SOURCES} ${MOD_HEADERS})

target_include_directories(CADMod PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
    ${OpenCASCADE_INCLUDE_DIR}
)

# CADCore, CADLogger, CADOCC, CADGeometry are PUBLIC because they may be used in public headers
target_link_libraries(CADMod PUBLIC
    CADCore
    CADLogger
    CADOCC
    CADGeometry
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

# CADConfig and CADRenderingToolkit are PRIVATE - only implementation needs them
target_link_libraries(CADMod PRIVATE
    CADConfig
    CADRenderingToolkit
    ${wxWidgets_LIBRARIES}
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

