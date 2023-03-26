cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
project(
  Awacorn
  VERSION 2.0.0
  LANGUAGES CXX)
add_library(Awacorn INTERFACE)
target_include_directories(Awacorn INTERFACE include/)

option(AWACORN_BUILD_EXAMPLE "Also build examples." ON)
if(AWACORN_BUILD_EXAMPLE)
  add_executable(timer include example/timer.cpp)
  target_link_libraries(timer PRIVATE Awacorn)
  add_executable(coroutine include example/coroutine.cpp)
  target_link_libraries(coroutine PRIVATE Awacorn)
endif()