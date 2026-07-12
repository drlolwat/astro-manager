qt_add_executable(profilegenerator-test
    profilegenerator_test.cpp
)
target_link_libraries(profilegenerator-test PRIVATE
    astro-spatial-core
    Qt6::Test
)
add_test(NAME profilegenerator-test COMMAND profilegenerator-test)

qt_add_executable(configdump
    configdump.cpp
)
target_link_libraries(configdump PRIVATE astro-spatial-core Qt6::Core)

qt_add_executable(pipewiremonitor-smoke
    pipewiremonitor_smoke.cpp
    ../src/pipewiremonitor.cpp
    ../src/pipewiremonitor.h
)
target_include_directories(pipewiremonitor-smoke PRIVATE ../src)
target_link_libraries(pipewiremonitor-smoke PRIVATE
    Qt6::Core
    PkgConfig::WirePlumber
)
