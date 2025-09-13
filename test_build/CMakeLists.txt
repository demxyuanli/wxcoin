cmake_minimum_required(VERSION 3.20)
project(SimpleTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)

# Create executable
add_executable(${PROJECT_NAME} ../simple_test.cpp)

# Link libraries
target_link_libraries(${PROJECT_NAME} ${wxWidgets_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(${PROJECT_NAME} PRIVATE ${wxWidgets_INCLUDE_DIRS})