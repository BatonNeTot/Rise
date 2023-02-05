# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

set(PROJECT_NAME "Rise")

project (${PROJECT_NAME})

add_subdirectory ("src")

file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS "include/*")

add_library (${PROJECT_NAME} STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_source_files_properties(${PROJECT_HEADERS} PROPERTIES HEADER_FILE_ONLY true)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/$<CONFIG>"
)

get_filename_component(RISE_WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
target_compile_definitions(${PROJECT_NAME} PRIVATE _AMD64_)

#Include setting RISE_RESOURCE_DIRECTORY in your CMake file
target_compile_definitions(${PROJECT_NAME} PUBLIC RISE_RESOURCE_DIRECTORY="${RISE_RESOURCE_DIRECTORY}")

add_subdirectory ("libs")
target_link_libraries(${PROJECT_NAME} debug shadercd shaderc_combinedd vulkan-1 glfw3 pugixml freetype)
target_link_libraries(${PROJECT_NAME} optimized shaderc shaderc_combined vulkan-1 glfw3 pugixml freetype)
target_include_directories(${PROJECT_NAME} PUBLIC "include")
target_include_directories(${PROJECT_NAME} PRIVATE "src")