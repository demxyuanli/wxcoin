set(WIDGETS_SRC
    # Flat UI Widgets (Core UI components)
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatLineEdit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatComboBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatCheckBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatSlider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatProgressBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatSwitch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatRadioButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatWidgetsExample.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatWidgetsExampleDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatWidgetsButtonsPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatWidgetsInputsPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatWidgetsSelectionPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatWidgetsProgressPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatTreeView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatTreeBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatEnhancedButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatMessagePanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FramelessModalPopup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/WebViewPanel.cpp

    # Modern Docking System
    ${CMAKE_CURRENT_SOURCE_DIR}/ModernDockManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ModernDockPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ModernDockAdapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DockGuides.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GhostWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DragDropController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LayoutEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DockSystemButtons.cpp
    
    # Layout Strategy Framework
    ${CMAKE_CURRENT_SOURCE_DIR}/layouts/FlexibleLayoutStrategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/layouts/HybridLayoutStrategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/layouts/LayoutStrategyFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/layouts/LayoutStrategyTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LayoutStrategyDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LayoutOptimizer.cpp
)

set(WIDGETS_HDRS
    # Flat UI Widgets (Core UI components)
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatButton.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatLineEdit.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatComboBox.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatCheckBox.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatSlider.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatProgressBar.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatSwitch.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatRadioButton.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatWidgetsExample.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatWidgetsExampleDialog.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatWidgetsButtonsPanel.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatWidgetsInputsPanel.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatWidgetsSelectionPanel.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatWidgetsProgressPanel.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatTreeView.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatTreeBuilder.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatEnhancedButton.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatMessagePanel.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FramelessModalPopup.h
    ${CMAKE_SOURCE_DIR}/include/WebViewPanel.h

    # Modern Docking System
    ${CMAKE_SOURCE_DIR}/include/widgets/ModernDockManager.h
    ${CMAKE_SOURCE_DIR}/include/widgets/ModernDockPanel.h
    ${CMAKE_SOURCE_DIR}/include/widgets/ModernDockAdapter.h
    ${CMAKE_SOURCE_DIR}/include/widgets/DockGuides.h
    ${CMAKE_SOURCE_DIR}/include/widgets/GhostWindow.h
    ${CMAKE_SOURCE_DIR}/include/widgets/DragDropController.h
    ${CMAKE_SOURCE_DIR}/include/widgets/LayoutEngine.h
    ${CMAKE_SOURCE_DIR}/include/widgets/DockTypes.h
    ${CMAKE_SOURCE_DIR}/include/widgets/DockSystemButtons.h
    
    # Unified Dock Framework Interfaces
    ${CMAKE_SOURCE_DIR}/include/widgets/UnifiedDockTypes.h
    ${CMAKE_SOURCE_DIR}/include/widgets/IDockManager.h
    ${CMAKE_SOURCE_DIR}/include/widgets/ILayoutStrategy.h
    ${CMAKE_SOURCE_DIR}/include/widgets/IVisualFeedback.h
    
    # Layout Strategies
    ${CMAKE_SOURCE_DIR}/include/widgets/layouts/FlexibleLayoutStrategy.h
    ${CMAKE_SOURCE_DIR}/include/widgets/layouts/HybridLayoutStrategy.h
    ${CMAKE_SOURCE_DIR}/include/widgets/layouts/LayoutStrategyFactory.h
    
    # Layout Strategy Dialog
    ${CMAKE_SOURCE_DIR}/include/widgets/LayoutStrategyDialog.h
    ${CMAKE_SOURCE_DIR}/include/widgets/LayoutOptimizer.h
)

add_library(widgets STATIC ${WIDGETS_SRC} ${WIDGETS_HDRS})

target_include_directories(widgets PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/widgets
    ${CMAKE_SOURCE_DIR}/include/widgets/layouts
    ${wxWidgets_INCLUDE_DIRS}
)

target_link_libraries(widgets PUBLIC
    ${wxWidgets_LIBRARIES}
)

# Enable precompiled headers for faster compilation
if(MSVC)
    target_precompile_headers(widgets PRIVATE
        <wx/wx.h>
        <wx/panel.h>
        <wx/button.h>
        <wx/sizer.h>
        <vector>
        <string>
        <memory>
    )
endif()

target_compile_definitions(widgets PRIVATE
    WXUSINGDLL
    __WXMSW__
)

target_compile_options(widgets PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

install(TARGETS widgets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)








