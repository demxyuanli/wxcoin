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

# Create executable
add_executable(docking_test ${TEST_SOURCES})

# Link libraries
target_link_libraries(docking_test
    ${wxWidgets_LIBRARIES}
    docking  # Link to the docking library built in parent project
)

# Set properties for Windows
if(WIN32)
    set_target_properties(docking_test PROPERTIES
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