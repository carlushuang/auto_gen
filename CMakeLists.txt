cmake_minimum_required(VERSION 3.0)
project(auto_gen)

option(OPENCL_BACKEND "Build with OpenCL backend (default)" ON)

# store all generated kernel in some directory
if("${KERNEL_DIR}_x" STREQUAL "_x")
	set(KERNEL_DIR ${CMAKE_CURRENT_BINARY_DIR}/Kernels/)
endif()
add_definitions(" -DKERNEL_DIR=\\\"${KERNEL_DIR}\\\"")

if(OPENCL_BACKEND)
	message(STATUS "Building with OpenCL backend")
	if(UNIX)
		if("${OCL_ROOT}_x" STREQUAL "_x")
			set(OCL_ROOT "/opt/rocm/opencl")	# default use rocm opencl
		endif()
		if(NOT EXISTS ${OCL_ROOT}/include/CL/opencl.h)
			message(FATAL_ERROR "OpenCL header not exist in ${OCL_ROOT}/include/CL/opencl.h")
		endif()
		if(NOT EXISTS ${OCL_ROOT}/lib/x86_64/libOpenCL.so)
			message(FATAL_ERROR "OpenCL library not exist in ${OCL_ROOT}/lib/x86_64/libOpenCL.so")
		endif()

		set(PROJECT_DEP_LIBS ${OCL_ROOT}/lib/x86_64/libOpenCL.so)
		set(PROJECT_DEP_INCLUDES ${OCL_ROOT}/include)
	endif()
	# search for "FindOpenCL.cmake" in CMAKE_MODULE_PATH.
	# find_package(OpenCL REQUIRED)
	# set(PROJECT_DEP_LIBS ${OPENCL_LIBRARIES})
	# set(PROJECT_DEP_INCLUDES ${OPENCL_INCLUDE_DIRS})
	add_definitions(" -DRUNTIME_OCL -DGPU_TIMER")
else()
	message(FATAL_ERROR "Unknown backend!")
endif()

file(GLOB PROJECT_SOURCES Benchmark/*.cpp)


macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

use_cxx11()

add_executable(auto_gen ${PROJECT_SOURCES})
target_include_directories(auto_gen PUBLIC ${PROJECT_DEP_INCLUDES})
target_link_libraries(auto_gen ${PROJECT_DEP_LIBS})
