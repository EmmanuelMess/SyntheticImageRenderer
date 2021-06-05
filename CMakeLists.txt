cmake_minimum_required(VERSION 3.19)
project(OgrePrintScreen)

set(CMAKE_CXX_STANDARD 20)

## [discover_ogre]
# specify which version and components you need
find_package(OGRE 1.11 REQUIRED COMPONENTS Bites RTShaderSystem)

add_executable(OgrePrintScreen main.cpp)

target_link_libraries(OgrePrintScreen ${OGRE_LIBRARIES})