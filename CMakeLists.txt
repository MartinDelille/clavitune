cmake_minimum_required(VERSION 3.16)
project(clavitune)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add the executable
add_executable(clavitune main.cpp)

# Link libraries
target_link_libraries(clavitune
  PRIVATE
  "-framework ApplicationServices"
)
