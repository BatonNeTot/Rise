# CMakeList.txt : CMake project for Rise, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS *.cpp *.h)
set(PROJECT_SOURCES "${PROJECT_SOURCES}" PARENT_SCOPE)
