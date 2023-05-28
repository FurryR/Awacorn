project(Awacorn_test LANGUAGES CXX)
include_directories(../include)
# Example
add_executable(timer example/timer.cpp)
add_executable(hello-world example/hello-world.cpp)
add_executable(then example/then.cpp)
# Test
add_executable(test-coroutine test-coroutine.cpp)
add_executable(test-promise performance/test-promise.cpp)
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
