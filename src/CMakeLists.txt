add_library(astro-spatial-core STATIC
    profilegenerator.cpp
    profilegenerator.h
    types.h
)
target_link_libraries(astro-spatial-core PUBLIC Qt6::Core)
target_include_directories(astro-spatial-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(astro-spatial
    main.cpp
    appcontroller.cpp
    appcontroller.h
    pipewiremonitor.cpp
    pipewiremonitor.h
    systemdmanager.cpp
    systemdmanager.h
)

qt_add_qml_module(astro-spatial
    URI AstroSpatial
    VERSION 1.0
    NO_IMPORT_SCAN
    QML_FILES
        qml/Main.qml
)

target_link_libraries(astro-spatial PRIVATE
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

install(TARGETS astro-spatial
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
