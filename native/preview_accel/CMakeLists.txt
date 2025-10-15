cmake_minimum_required(VERSION 3.18)
project(preview_accel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(preview_accel src/preview_accel.cpp)

# Produce a .pyd with no 'lib' prefix on Windows
set_target_properties(preview_accel PROPERTIES PREFIX "")
if (WIN32)
  set_target_properties(preview_accel PROPERTIES SUFFIX ".pyd")
endif()
