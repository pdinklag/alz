cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)
project(alz)

# hard requirements
find_package(OpenMP REQUIRED)
list(APPEND CMAKE_MODULE_PATH "deps/tbb/cmake/")
find_package(TBB REQUIRED)

# set C++ build flags
set(CXX_STANDARD c++23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=gnu++23 ${GCC_WARNINGS} ${OpenMP_CXX_FLAGS} -Wstringop-overflow=0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -march=native")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

# options
option(LIBSAIS_USE_OPENMP "Use OpenMP for parallelization" ON)

# dependencies
include(cmake/CPM.cmake)
CPMAddPackage("gh:IlyaGrebnov/libsais#v2.10.4")
CPMAddPackage("gh:martinus/unordered_dense#v4.8.1")
CPMAddPackage("gh:pdinklag/fp#v1.0.0")
CPMAddPackage("gh:pdinklag/iopp#v1.0.0")
CPMAddPackage("gh:pdinklag/lz77#v1.0.0")
CPMAddPackage("gh:pdinklag/pm#v1.0.0")

# ALZ library
add_library(alz INTERFACE)
target_include_directories(alz INTERFACE ${libsais_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(alz INTERFACE fp iopp lz77 libsais pm tbb unordered_dense)

# provide command-line tool if standalone
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    add_subdirectory(src)
endif()
