# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("Rise")

# Include sub-projects.
add_subdirectory ("src")

# install(SCRIPT "CMakeInstall.txt")

file(GLOB_RECURSE RES_FILES CONFIGURE_DEPENDS "res/*")

message("${RES_FILES}")
message("${CMAKE_CURRENT_BINARY_DIR}")

add_executable (Rise WIN32 ${RISE_SOURCES} ${RES_FILES})
set_property(TARGET Rise PROPERTY CXX_STANDARD 20)
set_source_files_properties(${RES_FILES} PROPERTIES HEADER_FILE_ONLY true)

get_filename_component(RISE_WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
target_compile_definitions(Rise PUBLIC RISE_RESOURCE_DIRECTORY="${RISE_WORKING_DIR}/res/")
target_compile_definitions(Rise PRIVATE _AMD64_)

#if(MSVC)
#    string(REGEX REPLACE "/W[1-3]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#else()
#    add_compile_options(-Wall -Wextra -pedantic -Werror)
#endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Rise)

add_subdirectory ("libs")
target_link_libraries(Rise debug shadercd shaderc_combinedd vulkan-1 glfw3 pugixml freetype)
target_link_libraries(Rise optimized shaderc shaderc_combined vulkan-1 glfw3 pugixml freetype)
target_include_directories(Rise INTERFACE "include")
target_include_directories(Rise PRIVATE "src")