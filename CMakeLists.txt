cmake_minimum_required(VERSION 3.24)

project(astro-manager VERSION 0.4.1 LANGUAGES CXX)

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
pkg_check_modules(LibUSB REQUIRED IMPORTED_TARGET libusb-1.0)

qt_standard_project_setup(REQUIRES 6.6)
qt_policy(SET QTP0004 NEW)

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(FILES packaging/io.github.drlolwat.AstroManager.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/60-astro-manager.rules
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/udev/rules.d)
install(FILES packaging/astro-manager.service
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user)
install(FILES packaging/io.github.drlolwat.AstroManager.service
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services)
install(FILES README.md NOTICE docs/FIRMWARE_RESEARCH.md
        DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/astro-manager)
install(CODE "file(REMOVE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/applications/io.github.drlolwat.AstroSpatial.desktop\")")
