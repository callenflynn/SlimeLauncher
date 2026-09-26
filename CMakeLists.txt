cmake_minimum_required(VERSION 3.22)
project(SlimeLauncher VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_resources(SLIME_RESOURCES src/resources.qrc)

add_executable(slime-launcher
    src/main.cpp
    src/PrismBridge.cpp
    src/ImageProcessor.cpp
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

# The .desktop entry ships Exec=slimelauncher (all-lowercase app id, per the
# freedesktop spec used by Rofi/dmenu/GNOME/KDE/Hyprland menus). Install the
# real binary as bin/slime-launcher and a slimelauncher symlink so both names
# resolve; StartupWMClass=slimelauncher must then match the window's app id.
# QCoreApplication::setApplicationName is "SlimeLauncher" (Constants::APP_ID),
# and Qt on X11 sets the WM class to lowercase first, so also force the
# lowercase app id on the QGuiApplication command line via main() before the
# window maps (kept in sync below).
install(CODE "
    set(_slime_bindir \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin)
    set(_slime_src \${_slime_bindir}/slime-launcher)
    set(_slime_dst \${_slime_bindir}/slimelauncher)
    if(EXISTS \${_slime_dst})
        file(REMOVE \${_slime_dst})
    endif()
    if(EXISTS \${_slime_src})
        file(CREATE_LINK \${_slime_src} \${_slime_dst})
    else()
        message(WARNING \"slime-launcher binary not found at \${_slime_src}; skipping slimelauncher link\")
    endif()
")

install(FILES packaging/slime-launcher.desktop DESTINATION share/applications)

# App icon: freedesktop hicolor entries for system menus (Rofi, GNOME App
# Grid, KDE Application Launcher, dmenu, Hyprland).
foreach(size 16 24 32 48 64 128 256)
    install(FILES packaging/icons/${size}x${size}/slime-launcher.png
            DESTINATION share/icons/hicolor/${size}x${size}/apps)
endforeach()
install(FILES packaging/icons/scalable/slime-launcher.svg
        DESTINATION share/icons/hicolor/scalable/apps)
install(FILES assets/slime.png DESTINATION share/icons RENAME slime.png)
install(FILES assets/slime.png DESTINATION share/slime-launcher RENAME slime.png)

# Default poster cards, shipped for release tarballs (the binary embeds them
# as Qt resources; these are the same files in unpacked form).
install(DIRECTORY assets/default_cards DESTINATION share/slime-launcher)

option(BUILD_TESTING "Build headless smoke tests" OFF)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
