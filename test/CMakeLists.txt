project(Awacorn_test LANGUAGES CXX)
include_directories(../include)
add_compile_options(-Wall -Wextra -std=c++2b -O3 -march=native)
# Example
add_executable(timer example/timer.cpp)
add_executable(hello-world example/hello-world.cpp)
# Test
add_executable(test-promise performance/test-promise.cpp)
add_executable(test-async performance/test-async.cpp)

add_test(NAME timer COMMAND timer)
add_test(NAME test-promise COMMAND test-promise)
add_test(NAME test-async COMMAND test-async)
