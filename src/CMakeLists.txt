add_library(astro-spatial-core STATIC
    a50device.cpp
    a50device.h
    a50protocol.cpp
    a50protocol.h
    profilegenerator.cpp
    profilegenerator.h
    profilestore.cpp
    profilestore.h
    types.h
)
target_link_libraries(astro-spatial-core PUBLIC Qt6::Core PkgConfig::LibUSB)
target_include_directories(astro-spatial-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(astro-manager
    main.cpp
    a50manager.cpp
    a50manager.h
    appcontroller.cpp
    appcontroller.h
    pipewiremonitor.cpp
    pipewiremonitor.h
    systemdmanager.cpp
    systemdmanager.h
)

qt_add_qml_module(astro-manager
    URI AstroManager
    VERSION 1.0
    NO_IMPORT_SCAN
    QML_FILES
        qml/Main.qml
        qml/DashboardPage.qml
        qml/SpatialPage.qml
        qml/HardwareEqPage.qml
        qml/LevelRow.qml
        qml/MicrophonePage.qml
        qml/StreamPage.qml
        qml/ProfilesPage.qml
        qml/DiagnosticsPage.qml
)

target_link_libraries(astro-manager PRIVATE
    astro-spatial-core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus
    KF6::Kirigami
    PkgConfig::WirePlumber
)

install(TARGETS astro-manager
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

qt_add_executable(astro-managerctl managerctl.cpp)
target_link_libraries(astro-managerctl PRIVATE Qt6::Core Qt6::DBus)
install(TARGETS astro-managerctl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(CODE "file(REMOVE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/astro-spatial\")")
install(CODE "file(CREATE_LINK astro-manager \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/astro-spatial\" SYMBOLIC)")
