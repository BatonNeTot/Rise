# CMakeList.txt : CMake project for Rise, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE RISE_SOURCES CONFIGURE_DEPENDS *.cpp *.h)
message ("${RISE_SOURCES}")
set(RISE_SOURCES "${RISE_SOURCES}" PARENT_SCOPE)
