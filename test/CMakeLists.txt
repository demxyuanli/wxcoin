cmake_minimum_required(VERSION 3.16)
project(DockingTest)

# Find required packages
find_package(wxWidgets REQUIRED COMPONENTS core base stc adv)
include(${wxWidgets_USE_FILE})

# Include docking library headers
include_directories(${CMAKE_SOURCE_DIR}/../include)

# Test application sources
set(TEST_SOURCES
    docking_test_app.cpp
    docking_test_app.h
)

# Integration test sources  
set(INTEGRATION_TEST_SOURCES
    test_docking_integration.cpp
)

# Create executables
add_executable(docking_test ${TEST_SOURCES})
add_executable(docking_integration_test ${INTEGRATION_TEST_SOURCES})

# Link libraries
target_link_libraries(docking_test
    ${wxWidgets_LIBRARIES}
    docking  # Link to the docking library built in parent project
)

target_link_libraries(docking_integration_test
    ${wxWidgets_LIBRARIES}
    UIFrame      # Main UI framework
    docking      # Docking library
    CADConfig    # Config manager
    CADLogger    # Logger
)

# Set properties for Windows
if(WIN32)
    set_target_properties(docking_test PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    set_target_properties(docking_integration_test PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Copy any required resources
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/resources)
    add_custom_command(TARGET docking_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:docking_test>/resources
    )
endif()