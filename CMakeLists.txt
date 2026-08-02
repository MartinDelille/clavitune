cmake_minimum_required(VERSION 3.16)
project(clavitune)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include directories
include_directories(/opt/homebrew/include)

# Link directories
link_directories(/opt/homebrew/lib)

# Add the executable
add_executable(clavitune main.cpp)

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
