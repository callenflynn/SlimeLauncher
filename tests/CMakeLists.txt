# Smoke tests — built only with -DBUILD_TESTING=ON; never installed.
cmake_minimum_required(VERSION 3.22)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core)

add_executable(prism-bridge-smoke PrismBridgeSmokeTest.cpp)
target_include_directories(prism-bridge-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(prism-bridge-smoke PRIVATE Qt6::Core)

# PrismBridge.cpp is compiled directly into the test (it has no UI deps).
target_sources(prism-bridge-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/PrismBridge.cpp)
set_target_properties(prism-bridge-smoke PROPERTIES AUTOMOC ON)
