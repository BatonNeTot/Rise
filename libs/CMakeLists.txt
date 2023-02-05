# CMakeList.txt : CMake project for Rise, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

file(GLOB LIBS_DIRECTORIES LIST_DIRECTORIES true *)
file(GLOB LIBS_FILES LIST_DIRECTORIES false *)

foreach(LIBS_FILE ${LIBS_FILES})
	list(REMOVE_ITEM LIBS_DIRECTORIES ${LIBS_FILE})
endforeach()

foreach(LIBS_DIRECTORY ${LIBS_DIRECTORIES})
	message("${LIBS_DIRECTORY}")
	if(EXISTS "${LIBS_DIRECTORY}/include")
		target_include_directories(Rise PUBLIC "${LIBS_DIRECTORY}/include")
	endif()
	if(EXISTS "${LIBS_DIRECTORY}/lib")
		target_link_directories(Rise PUBLIC "${LIBS_DIRECTORY}/lib")
	endif()
endforeach()
