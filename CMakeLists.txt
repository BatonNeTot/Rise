# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("Rise")
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE PROJECT_SOURCES
            CONFIGURE_DEPENDS
            Rise/src/*.c
            Rise/src/*.cpp)

add_executable (Rise ${PROJECT_SOURCES})

target_include_directories(Rise PUBLIC "Rise/include")
target_include_directories(Rise PRIVATE "Rise/src")

target_include_directories(Rise PUBLIC "C:/VulkanSDK/1.2.162.0/Include")
target_include_directories(Rise PUBLIC "dependencies/glfw/include")
target_include_directories(Rise PUBLIC "dependencies/glm")

target_link_directories(Rise PUBLIC "C:/VulkanSDK/1.2.162.0/Lib")
target_link_directories(Rise PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/dependencies/glfw/src")

add_subdirectory(dependencies/glfw dependencies/glfw EXCLUDE_FROM_ALL)
add_dependencies(Rise glfw)

target_link_libraries(Rise PUBLIC vulkan-1 PUBLIC glfw3)