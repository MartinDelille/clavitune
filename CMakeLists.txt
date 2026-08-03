cmake_minimum_required(VERSION 3.16)
project(clavitune)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add the executable
add_executable(clavitune main.cpp)

if(APPLE)
  # Include directories
  include_directories(/opt/homebrew/include)

  # Link directories
  target_link_directories(clavitune PRIVATE /opt/homebrew/lib)

  add_compile_definitions(__OS_MACOSX__)

  # Link libraries
  target_link_libraries(clavitune
    PRIVATE
      stk
    "-framework ApplicationServices"
    "-framework CoreFoundation"
    "-framework CoreAudio"
    "-framework AudioToolbox"
  )
elseif(UNIX)
  find_package(ALSA REQUIRED)
  find_package(Threads REQUIRED)

  include(FetchContent)
  set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
  set(COMPILE_PROJECTS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    stk
    GIT_REPOSITORY https://github.com/thestk/stk.git
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(stk)

  # STK's own headers only end up under an `stk/` prefix once installed
  # (its CMakeLists installs to include/stk); the fetched source tree has
  # them flat under include/. Bridge that with a symlink so `#include
  # <stk/ADSR.h>` resolves the same way it does via Homebrew on macOS.
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/stk_include)
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/stk_include/stk)
    file(CREATE_LINK ${stk_SOURCE_DIR}/include
         ${CMAKE_BINARY_DIR}/stk_include/stk SYMBOLIC)
  endif()

  add_compile_definitions(__LINUX_ALSA__)

  target_include_directories(clavitune PRIVATE ${CMAKE_BINARY_DIR}/stk_include)
  target_link_libraries(clavitune
    PRIVATE
      stk
      ALSA::ALSA
      Threads::Threads
  )
endif()
