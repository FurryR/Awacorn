cmake_minimum_required(VERSION 3.10)
project(
  awacorn
  VERSION 3.1.0
  LANGUAGES CXX)
add_library(awacorn INTERFACE)
set(CMAKE_CXX_STANDARD 11)
target_compile_features(awacorn INTERFACE cxx_std_11)
target_include_directories(awacorn
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
include(CMakeDependentOption)
option(AWACORN_BUILD_EXAMPLE "Also build examples (and tests)." ON)
cmake_dependent_option(AWACORN_USE_BOOST "Use boost as generator." OFF
                       "AWACORN_USE_UCONTEXT" OFF)
cmake_dependent_option(AWACORN_USE_UCONTEXT "Use ucontext as generator." OFF
                       "AWACORN_USE_BOOST" OFF)
if(NOT AWACORN_USE_BOOST AND NOT AWACORN_USE_UCONTEXT)
  if(Boost_FOUND)
    set(AWACORN_USE_BOOST ON)
  else()
    if(NOT Boost_FOUND)
      find_package(Boost REQUIRED COMPONENTS coroutine context)
    endif()
    if(Boost_FOUND)
      set(AWACORN_USE_BOOST ON)
    else()
      set(AWACORN_USE_UCONTEXT ON)
    endif()
  endif()
endif()
if(AWACORN_USE_BOOST)
  if(NOT Boost_FOUND)
    find_package(Boost REQUIRED COMPONENTS coroutine context)
  endif()
  if(NOT Boost_FOUND)
    message(FATAL_ERROR "boost not found, failed")
  endif()
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
  message("Using boost as generator.")
  add_definitions(-DAWACORN_USE_BOOST)
elseif(AWACORN_USE_UCONTEXT)
  message("Using ucontext as generator.")
  add_definitions(-DAWACORN_USE_UCONTEXT)
else()
  message(FATAL_ERROR "did not specify a library, failed")
endif()
if(AWACORN_BUILD_EXAMPLE)
  add_compile_options(-Wall -Wextra -g)
  # Example
  add_executable(timer include example/timer.cpp)
  add_executable(hello-world include example/hello-world.cpp)
  add_executable(then include example/then.cpp)
  # Test
  add_executable(test-coroutine include test/test-coroutine.cpp)
  add_executable(test-promise include test/performance/test-promise.cpp)
  if(AWACORN_USE_BOOST)
    # Example
    target_link_libraries(timer PRIVATE awacorn boost_coroutine boost_context)
    target_link_libraries(hello-world PRIVATE awacorn boost_coroutine
                                              boost_context)
    target_link_libraries(then PRIVATE awacorn boost_coroutine boost_context)
    # Test
    target_link_libraries(test-coroutine PRIVATE awacorn boost_coroutine
                                                 boost_context)
    target_link_libraries(test-promise PRIVATE awacorn boost_coroutine
                                               boost_context)
  else()
    # Example
    target_link_libraries(timer PRIVATE awacorn)
    target_link_libraries(hello-world PRIVATE awacorn)
    target_link_libraries(then PRIVATE awacorn)
    # Test
    target_link_libraries(test-coroutine PRIVATE awacorn)
    target_link_libraries(test-promise PRIVATE awacorn)
  endif()
endif()
install()