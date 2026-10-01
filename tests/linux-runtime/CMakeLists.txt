cmake_minimum_required(VERSION 3.24)
project(EmulosLinuxRuntimeTests LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick Quick3D Concurrent Network Test)
get_filename_component(ROOT "../.." ABSOLUTE)
qt_add_executable(linux_runtime_tests runtime_tests.cpp
    ${ROOT}/app/game_importer.cpp ${ROOT}/app/library.cpp
    ${ROOT}/app/avatar_studio.cpp ${ROOT}/app/player_services.cpp
    ${ROOT}/app/core_window_host.cpp ${ROOT}/app/engine_settings.cpp
    ${ROOT}/app/console_config.cpp ${ROOT}/app/save_store.cpp
    ${ROOT}/app/community_package.cpp
    ${ROOT}/engine/xenia/src/xenia/cpu/lzx.cc
    ${ROOT}/engine/xenia/third_party/mspack/lzxd.c)
target_include_directories(linux_runtime_tests PRIVATE ${ROOT}/app ${ROOT}/engine/xenia
    ${ROOT}/engine/xenia/src ${ROOT}/engine/xenia/third_party/mspack
    ${ROOT}/engine/xenia/third_party/fmt/include)
target_compile_definitions(linux_runtime_tests PRIVATE FMT_HEADER_ONLY)
target_link_libraries(linux_runtime_tests PRIVATE Qt6::Quick Qt6::Quick3D Qt6::Concurrent Qt6::Network Qt6::Test)
qt_add_resources(linux_runtime_tests catalog BASE ${ROOT} PREFIX "/" FILES ${ROOT}/app/settings-catalog.json)
option(EMULOS_SANITIZE "Address and undefined behavior checks" ON)
if(EMULOS_SANITIZE)
    target_compile_options(linux_runtime_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(linux_runtime_tests PRIVATE -fsanitize=address,undefined)
endif()
