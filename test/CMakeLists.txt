project(Awacorn_test LANGUAGES CXX)
include_directories(../include)
add_compile_options(-Wall -Wextra -std=c++11)
# Example
add_executable(timer example/timer.cpp)
add_executable(hello-world example/hello-world.cpp)
add_executable(then example/then.cpp)
# Test
add_executable(test-promise performance/test-promise.cpp)

add_test(NAME timer COMMAND timer)
add_test(NAME then COMMAND then)
add_test(NAME test-promise COMMAND test-promise)
