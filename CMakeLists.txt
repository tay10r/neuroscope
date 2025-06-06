cmake_minimum_required(VERSION 3.20)

project(neuroscope)

find_package(OpenMP REQUIRED COMPONENTS CXX)
find_package(embree REQUIRED)

add_library(neuroscope_cpp
  core.h
  core.cpp
  scene.h
  scene.cpp
  microscope.h
  microscope.cpp
  swc.h
  swc.cpp
)

if(COMPILER_WARNINGS_ENABLED)
  if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(neuroscope_cpp PUBLIC -Wall -Wextra)
  endif()
endif()

set_target_properties(neuroscope_cpp
  PROPERTIES
    OUTPUT_NAME neuroscope
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(neuroscope_cpp
  PUBLIC
    OpenMP::OpenMP_CXX
    embree
)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

set(PYBIND11_FINDPYTHON NEW)

FetchContent_Declare(pybind11
  URL "https://github.com/pybind/pybind11/archive/refs/tags/v3.0.0rc2.zip"
  URL_HASH "SHA256=3846243e173670be67e04c4bdaceb125a127846397afd721793fc60718760793"
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(neuroscope python.cpp)

target_link_libraries(neuroscope PUBLIC neuroscope_cpp)

install(TARGETS neuroscope
  RUNTIME DESTINATION .
  LIBRARY DESTINATION .
  ARCHIVE DESTINATION .
)
