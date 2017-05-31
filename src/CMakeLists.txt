cmake_minimum_required(VERSION 2.8)

add_executable(pong pong.cpp)
add_executable(shader shader.cpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /usr/share/SFML/cmake/Modules)
find_package(SFML 2.4 REQUIRED system window graphics network audio)
if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(pong ${SFML_LIBRARIES})
  target_link_libraries(shader ${SFML_LIBRARIES})
endif()

