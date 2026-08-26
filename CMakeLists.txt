cmake_minimum_required(VERSION 3.21)
project(CommonLibF4RD)

option(COMMONLIBF4RD_BUILD_EXAMPLE "Build the included example plugin" ${PROJECT_IS_TOP_LEVEL})

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

function(conditionally_add_subdirectory SRC_DIR)
	if(NOT TARGET "${SRC_DIR}")
		if("${ARGC}" GREATER 1)
			add_subdirectory("${SRC_DIR}" "${ARGV1}")
		else()
			add_subdirectory("${SRC_DIR}")
		endif()
	endif()
endfunction()

conditionally_add_subdirectory(CommonLibF4)
if(COMMONLIBF4RD_BUILD_EXAMPLE)
	conditionally_add_subdirectory(ExampleProject)
endif()
