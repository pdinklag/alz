cmake_minimum_required(VERSION 3.28.0 FATAL_ERROR)

# set build type to Release if nothing was specified
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE "Release" FORCE)
endif()

project(alz)

# hard requirements
find_package(OpenMP REQUIRED)

# TODO: make TBB dependency optional
set(ENABLE_PARALLEL_SORT 0)
include(CheckCXXSymbolExists)
check_cxx_symbol_exists(__cpp_lib_parallel_algorithm "version" HAVE_CPP_LIB_PARALLEL_ALGORITHM)
if(HAVE_CPP_LIB_PARALLEL_ALGORITHM)
  # the compiler supports parallel execution policies
  # make sure TBB is installed
  list(APPEND CMAKE_MODULE_PATH "deps/tbb/cmake/")
  find_package(TBB)
  if(TBB_FOUND)
    set(TBB_LIBRARY "tbb")
    set(ENABLE_PARALLEL_SORT 1 FORCE)
  else()
    message("TBB library not found -- parallel sorting is disabled.")
  endif()
else()
  message("The compiler does not support parallel execution policies (__cpp_lib_parallel_algorithm not set) -- parallel sorting is disabled.")
  set(TBB_LIBRARY "")
endif()

# set C++ build flags
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

# options
option(LIBSAIS_USE_OPENMP "Use OpenMP for parallelization" ON)

# dependencies
include(cmake/CPM.cmake)
CPMAddPackage("gh:IlyaGrebnov/libsais#v2.10.4")
CPMAddPackage("gh:martinus/unordered_dense#v4.8.1")
CPMAddPackage("gh:pdinklag/fp#v1.0.1")
CPMAddPackage("gh:pdinklag/iopp#v1.0.1")
CPMAddPackage("gh:pdinklag/lz77#v1.0.4")
CPMAddPackage("gh:pdinklag/pm#v1.0.1")

# ALZ library
add_library(alz INTERFACE)
target_include_directories(alz INTERFACE ${libsais_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(alz INTERFACE fp iopp lz77 libsais pm ${TBB_LIBRARY} unordered_dense)

if(ENABLE_PARALLEL_SORT)
  target_compile_definitions(alz INTERFACE ALZ_PARALLEL_SORT_ENABLED)
endif()

# provide command-line tool if standalone
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    add_subdirectory(src)
endif()
