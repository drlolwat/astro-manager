cmake_minimum_required(VERSION 3.24)

project(astro-spatial VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Widgets Quick QuickControls2 DBus Test)
find_package(KF6Kirigami REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WirePlumber REQUIRED IMPORTED_TARGET wireplumber-0.5)

qt_standard_project_setup(REQUIRES 6.6)
qt_policy(SET QTP0004 NEW)

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(FILES packaging/io.github.drlolwat.AstroSpatial.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/io.github.drlolwat.AstroSpatial-autostart.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/astro-spatial/autostart)
