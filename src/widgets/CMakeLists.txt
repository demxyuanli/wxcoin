set(WIDGETS_SRC
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
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatTreeView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatEnhancedButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatDockContainer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatDockCaptionBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatDockFloatFrame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatDockManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatMessagePanel.cpp
)

set(WIDGETS_HDRS
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
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatTreeView.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatEnhancedButton.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatDockContainer.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatDockCaptionBar.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatDockFloatFrame.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatDockManager.h
    ${CMAKE_SOURCE_DIR}/include/widgets/FlatMessagePanel.h
)

add_library(widgets STATIC ${WIDGETS_SRC} ${WIDGETS_HDRS})

target_include_directories(widgets PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
)

target_link_libraries(widgets PUBLIC
    ${wxWidgets_LIBRARIES}
)

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
