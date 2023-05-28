cmake_minimum_required(VERSION 3.10)
project(
  Awacorn
  VERSION 3.1.0
  LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakeDependentOption)
cmake_dependent_option(USE_BOOST "Use boost_context as generator." OFF
                       "USE_UCONTEXT" OFF)
cmake_dependent_option(USE_UCONTEXT "Use ucontext as generator." OFF
                       "USE_BOOST" OFF)
if(NOT USE_BOOST AND NOT USE_UCONTEXT)
  if(Boost_FOUND)
    set(USE_BOOST ON)
  else()
    if(NOT Boost_FOUND)
      find_package(Boost REQUIRED COMPONENTS context)
    endif()
    if(Boost_FOUND)
      set(USE_BOOST ON)
    else()
      set(USE_UCONTEXT ON)
    endif()
  endif()
endif()
if(USE_BOOST)
  if(NOT Boost_FOUND)
    find_package(Boost REQUIRED COMPONENTS context)
  endif()
  if(NOT Boost_FOUND)
    message(
      FATAL_ERROR
        "Awacorn does not work properly without \"boost_context\". Please install package \"boost_context\" to continue."
    )
  endif()
  message("Awacorn is using \"boost_context\" as coroutine.")
elseif(USE_UCONTEXT)
  message("Awacorn is using \"ucontext\" as coroutine.")
endif()
# 创建 Awacorn 库
add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_11)
target_include_directories(
  ${PROJECT_NAME}
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
# 安装 targets
install(
  TARGETS ${PROJECT_NAME}
  EXPORT AwacornTargets
  INCLUDES ${CMAKE_INSTALL_INCLUDEDIR})
# 安装头文件
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/awacorn)
# 导出并安装 export 文件
install(
  EXPORT ${PROJECT_NAME}Targets
  FILE ${PROJECT_NAME}Targets.cmake
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/awacorn)
include(CMakePackageConfigHelpers)
# 创建版本
write_basic_package_version_file(
  "${PROJECT_NAME}ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)
if(NOT INCLUDE_INSTALL_DIR)
  set(INCLUDE_INSTALL_DIR include)
endif()
# 创建 config
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/awacorn
  PATH_VARS INCLUDE_INSTALL_DIR)
# 安装 config
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
              "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/awacorn)
# 生成导出 targets
export(EXPORT ${PROJECT_NAME}Targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Targets.cmake")
# 启动测试
include(CTest)
option(BUILD_TEST "Also build examples and tests." ON)
if(${BUILD_TEST})
  enable_testing()
  add_subdirectory(test)
endif()
