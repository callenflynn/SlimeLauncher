cmake_minimum_required(VERSION 3.22)
project(SlimeLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_resources(SLIME_RESOURCES src/resources.qrc)

add_executable(slime-launcher
    src/main.cpp
    src/PrismBridge.cpp
    src/GamepadFilter.cpp
    src/ThemeManager.cpp
    src/InstanceCard.cpp
    src/LogViewer.cpp
    src/ErrorPanel.cpp
    src/SideNav.cpp
    src/SetupWizard.cpp
    src/DashboardWindow.cpp
    ${SLIME_RESOURCES}
)

target_include_directories(slime-launcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(slime-launcher PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

target_compile_options(slime-launcher PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CXX_COMPILER_ID:GNU>:-Wno-unused-parameter>
)

install(TARGETS slime-launcher RUNTIME DESTINATION bin)
install(FILES packaging/slime-launcher.desktop DESTINATION share/applications)

# Slime block logo: freedesktop hicolor icon theme entries + a plain copy
# alongside the binary for release-tarball use.
foreach(size 16 24 32 48 64 128 256)
    install(FILES packaging/icons/${size}x${size}/slime-launcher.png
            DESTINATION share/icons/hicolor/${size}x${size}/apps)
endforeach()
install(FILES assets/slime.png DESTINATION share/icons RENAME slime.png)
install(FILES assets/slime.png DESTINATION share/slime-launcher RENAME slime.png)

option(BUILD_TESTING "Build headless smoke tests" OFF)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
