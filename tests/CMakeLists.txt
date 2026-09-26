# Smoke tests — built only with -DBUILD_TESTING=ON; never installed.
cmake_minimum_required(VERSION 3.22)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui)

add_executable(prism-bridge-smoke PrismBridgeSmokeTest.cpp)
target_include_directories(prism-bridge-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(prism-bridge-smoke PRIVATE Qt6::Core Qt6::Gui)

# PrismBridge.cpp + ImageProcessor.cpp are compiled directly into the test
# (neither has UI deps). The default poster cards are embedded too — the
# pipeline seeding reads them from :/cards/.
qt_add_resources(SLIME_TEST_RCC ${CMAKE_CURRENT_SOURCE_DIR}/../src/resources.qrc)
target_sources(prism-bridge-smoke PRIVATE ${SLIME_TEST_RCC})
target_sources(prism-bridge-smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PrismBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ImageProcessor.cpp
)
set_target_properties(prism-bridge-smoke PROPERTIES AUTOMOC ON)
