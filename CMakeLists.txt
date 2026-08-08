cmake_minimum_required(VERSION 3.16)
project(clavitune)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(stk CONFIG REQUIRED)

# Add the executable
add_executable(clavitune main.cpp)

if(APPLE)
  add_compile_definitions(__OS_MACOSX__)

  # Link libraries
  target_link_libraries(clavitune
    PRIVATE
      stk::stk
    "-framework ApplicationServices"
    "-framework CoreFoundation"
    "-framework CoreAudio"
    "-framework AudioToolbox"
  )
elseif(UNIX)
  find_package(ALSA CONFIG REQUIRED)
  find_package(Threads REQUIRED)

  add_compile_definitions(__LINUX_ALSA__)

  target_link_libraries(clavitune
    PRIVATE
      stk::stk
      ALSA::ALSA
      Threads::Threads
  )
endif()
