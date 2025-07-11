# FlatUI module - modern flat UI components and framework
set(FLATUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIBarDrawing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIBarEventDispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIBarLayoutManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIBarPerformanceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIBarSpaces.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIBarStateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIBarTabs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIButtonBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUICustomControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIEventManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIFixPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIFloatPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIFrame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIFunctionSpace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIGallery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIHomeMenu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIHomeSpace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIPageManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIPinButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIProfileSpace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUISpacerControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUISystemButtons.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUITabDropdown.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatUIUnpinButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BorderlessFrameLogic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomDropDown.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UIHierarchyDebugger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatBarSpaceContainer.cpp
)

set(FLATUI_HEADERS
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIBar.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIBarConfig.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIBarEventDispatcher.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIBarLayoutManager.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIBarPerformanceManager.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIBarStateManager.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIButtonBar.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUICustomControl.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIEventManager.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIFixPanel.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIFloatPanel.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIFrame.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIFunctionSpace.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIGallery.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIHomeMenu.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIHomeSpace.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIPage.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIPageManager.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIPanel.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIPinButton.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIPinControl.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIProfileSpace.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUISpacerControl.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUISystemButtons.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUITabDropdown.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatUIUnpinButton.h
    ${CMAKE_SOURCE_DIR}/include/flatui/BorderlessFrameLogic.h
    ${CMAKE_SOURCE_DIR}/include/flatui/CustomDropDown.h
    ${CMAKE_SOURCE_DIR}/include/flatui/UIHierarchyDebugger.h
    ${CMAKE_SOURCE_DIR}/include/flatui/FlatBarSpaceContainer.h
)

# Create FlatUI static library
add_library(CADFlatUI STATIC ${FLATUI_SOURCES} ${FLATUI_HEADERS})

# Set header directories
target_include_directories(CADFlatUI PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(CADFlatUI PUBLIC
    CADCore
    CADConfig
    CADLogger
    ${wxWidgets_LIBRARIES}
    Coin::Coin
)

# Set library export properties
set_target_properties(CADFlatUI PROPERTIES
    FOLDER "FlatUI"
    EXPORT_NAME FlatUI
) 