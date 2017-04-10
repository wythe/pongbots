cmake_minimum_required(VERSION 2.8)

if(NOT WIN32)
add_definitions(-std=c++14)
endif()

add_executable(pong64 pong64.cpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /usr/share/SFML/cmake/Modules)
find_package(SFML 2 REQUIRED system window graphics network audio)
if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(pong64 ${SFML_LIBRARIES})
endif()

