cmake_minimum_required(VERSION 3.20)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" FZERO_VERSION LIMIT_COUNT 1)
project(FZeroSNESRecomp VERSION ${FZERO_VERSION} LANGUAGES C)
set(CMAKE_C_STANDARD 11)

include(CTest)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FzeroSharedForceDependencies.cmake")
option(FZERO_BUILD_GAME "Build game hosts (requires privately generated ROM sources)" ON)
set(RECOMP_UI_ROOT "${CMAKE_SOURCE_DIR}/recomp-ui" CACHE PATH
    "Path to the shared recomp-ui launcher checkout")
set(FZERO_SETUP_BUNDLE "" CACHE PATH "Qualified private standalone controls bundle; optional, no default setup action")
set(FZERO_SETUP_IDENTITY "${CMAKE_BINARY_DIR}/generated/fzero_setup_identity.h")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
if(FZERO_SETUP_BUNDLE)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/tools/setup_bundle.py" "${FZERO_SETUP_BUNDLE}/bundle-manifest.json")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/setup_bundle.py"
        --bundle "${FZERO_SETUP_BUNDLE}" --header "${FZERO_SETUP_IDENTITY}"
        RESULT_VARIABLE FZERO_SETUP_ADMISSION)
    if(NOT FZERO_SETUP_ADMISSION EQUAL 0)
        message(FATAL_ERROR "Private controls setup bundle admission failed")
    endif()
else()
    file(WRITE "${FZERO_SETUP_IDENTITY}" "#pragma once\n#define FZERO_SETUP_QUALIFIED 0\n#define FZERO_SETUP_REPORT_QUALIFIED 0\n#define FZERO_SETUP_SOURCE_COMMIT \"\"\n#define FZERO_SETUP_MANIFEST_SHA256 \"\"\n#define FZERO_SETUP_EXE_SHA256 \"\"\n#define FZERO_SETUP_MANIFEST_BYTES 0\n#define FZERO_SETUP_EXE_BYTES 0\n")
endif()
add_library(fzero_video STATIC src/fzero_video.c src/fzero_mode7.c
    src/fzero_replay.c src/fzero_hdma.c src/fzero_triple_display.c src/fzero_panel.c)
target_include_directories(fzero_video PUBLIC "${CMAKE_SOURCE_DIR}/src")
if(NOT MSVC)
    target_compile_options(fzero_video PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(fzero_video PUBLIC m)
endif()
if(BUILD_TESTING)
    enable_language(CXX)
    add_executable(fzero_controls_contract_tests tests/test_controls_contract.cpp)
    target_include_directories(fzero_controls_contract_tests PRIVATE "${CMAKE_SOURCE_DIR}/src" "${RECOMP_UI_ROOT}/src/common")
    target_compile_features(fzero_controls_contract_tests PRIVATE cxx_std_17)
    add_test(NAME fzero_controls_contract COMMAND fzero_controls_contract_tests
        "${CMAKE_SOURCE_DIR}/src/vendor/controls/controls-vectors.txt")
    add_executable(fzero_frame_stats_tests tests/test_frame_stats.cpp src/fzero_frame_stats.cpp)
    target_include_directories(fzero_frame_stats_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
    target_compile_features(fzero_frame_stats_tests PRIVATE cxx_std_17)
    add_test(NAME fzero_frame_stats COMMAND fzero_frame_stats_tests)
    add_executable(fzero_session_ui_tests tests/test_session_ui.c src/fzero_session_ui.c src/fzero_playthrough.c)
    target_include_directories(fzero_session_ui_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
    add_test(NAME fzero_session_ui COMMAND fzero_session_ui_tests)
    if(WIN32)
        add_executable(fzero_session_control_tests tests/test_session_control.c
            src/fzero_session_control.c src/fzero_session_ui.c src/fzero_playthrough.c)
        target_include_directories(fzero_session_control_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
        add_test(NAME fzero_session_control COMMAND fzero_session_control_tests)
    endif()
    add_executable(fzero_shim_map_tests tests/test_shim_map.c src/fzero_shim_map.c
        src/fzero_analog.c src/fzero_playthrough.c)
    target_include_directories(fzero_shim_map_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
    if(NOT MSVC)
        target_link_libraries(fzero_shim_map_tests PRIVATE m)
        target_compile_options(fzero_shim_map_tests PRIVATE -Wall -Wextra -Werror)
    endif()
    add_test(NAME fzero_shim_map COMMAND fzero_shim_map_tests)
    if(WIN32)
        enable_language(CXX)
        add_executable(fzero_setup_tests tests/test_setup.cpp)
        target_compile_features(fzero_setup_tests PRIVATE cxx_std_17)
        target_include_directories(fzero_setup_tests PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/tests")
        target_link_libraries(fzero_setup_tests PRIVATE bcrypt shell32)
        add_test(NAME fzero_setup COMMAND fzero_setup_tests)
        if(NOT FZERO_SETUP_BUNDLE)
            # Compile the real no-bundle generated header, not the inert qualified fixture.
            add_executable(fzero_setup_no_bundle_tests tests/test_setup_no_bundle.cpp src/fzero_setup.cpp)
            target_compile_features(fzero_setup_no_bundle_tests PRIVATE cxx_std_17)
            target_include_directories(fzero_setup_no_bundle_tests PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}/generated")
            target_link_libraries(fzero_setup_no_bundle_tests PRIVATE bcrypt)
            add_test(NAME fzero_setup_no_bundle COMMAND fzero_setup_no_bundle_tests)
        endif()
        add_executable(fzero_shared_owner_tests tests/test_shared_owner.cpp src/fzero_shared_owner.cpp)
        target_compile_features(fzero_shared_owner_tests PRIVATE cxx_std_17)
        target_include_directories(fzero_shared_owner_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
        add_test(NAME fzero_shared_owner COMMAND fzero_shared_owner_tests)
        add_executable(fzero_shared_ffb_tests tests/test_ffb_lifecycle.cpp src/fzero_shared_owner.cpp
            src/fzero_hotkeys.c src/fzero_output_latch.c src/fzero_inject.cpp src/fzero_controls.cpp
            src/fzero_shim.cpp src/fzero_shim_map.c src/fzero_analog.c src/fzero_telemetry.cpp)
        target_link_libraries(fzero_shared_ffb_tests PRIVATE ws2_32)
        target_compile_definitions(fzero_shared_ffb_tests PRIVATE FZERO_SHARED_CORE_TEST)
        set_target_properties(fzero_shared_ffb_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/shim-fixture")
        add_dependencies(fzero_shared_ffb_tests fzero_shim_fake)
        target_compile_features(fzero_shared_ffb_tests PRIVATE cxx_std_17)
        foreach(mode shared-owner shared-missing shared-core shared-core-extras injection-latch force-first checked-refusal checked-loss)
            add_test(NAME fzero_shared_ffb_${mode} COMMAND fzero_shared_ffb_tests --${mode})
        endforeach()
        add_test(NAME fzero_shared_ffb_legacy COMMAND fzero_shared_ffb_tests)
        # Keep the fake DLL away from the game's real vendored runtime.
        add_library(fzero_shim_fake SHARED tests/fake_shim.cpp)
        set_target_properties(fzero_shim_fake PROPERTIES OUTPUT_NAME dbce_shim
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/shim-fixture")
        target_compile_features(fzero_shim_fake PRIVATE cxx_std_17)
        add_executable(fzero_shim_host_tests tests/test_shim_host.cpp src/fzero_shim.cpp
            src/fzero_shim_map.c src/fzero_analog.c src/fzero_hotkeys.c src/fzero_output_latch.c
            src/fzero_ffb.cpp src/fzero_shared_owner.cpp src/fzero_telemetry.cpp)
        target_compile_definitions(fzero_shim_host_tests PRIVATE FZERO_FFB_MODEL_ONLY)
        target_link_libraries(fzero_shim_host_tests PRIVATE ws2_32)
        target_compile_features(fzero_shim_host_tests PRIVATE cxx_std_17)
        target_include_directories(fzero_shim_host_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
        set_target_properties(fzero_shim_host_tests PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/shim-fixture")
        add_dependencies(fzero_shim_host_tests fzero_shim_fake)
        foreach(mode off args replay normal abi pending record stop fences latched expired provenance session hostcontrols)
            add_test(NAME fzero_shim_host_${mode} COMMAND fzero_shim_host_tests ${mode})
        endforeach()
        add_executable(fzero_shim_ordinary_tests tests/test_shim_ordinary.cpp src/fzero_shim.cpp
            src/fzero_shim_map.c src/fzero_analog.c src/fzero_hotkeys.c src/fzero_output_latch.c
            src/fzero_ffb.cpp src/fzero_shared_owner.cpp src/fzero_telemetry.cpp src/fzero_playthrough.c)
        target_compile_definitions(fzero_shim_ordinary_tests PRIVATE FZERO_FFB_MODEL_ONLY)
        target_link_libraries(fzero_shim_ordinary_tests PRIVATE ws2_32)
        target_compile_features(fzero_shim_ordinary_tests PRIVATE cxx_std_17)
        target_include_directories(fzero_shim_ordinary_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
        set_target_properties(fzero_shim_ordinary_tests PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/shim-fixture")
        add_dependencies(fzero_shim_ordinary_tests fzero_shim_fake)
        foreach(mode normal native replay force forcechanged unknown session profile profileinfo missing unexpected launcher
            extras extrasmissing extrasseq extrascount extrasabi extrasinfo extrasnosection extrasduplicate extrasfirst extrasbom
            configlimit configoversize extrasconfiglimit extrasconfigoversize
            launchersaved launcherempty launcherbackendchanged launcherbadextras launcherrecord
            hosts hostsnoflag hostsrecord hostschanged)
            add_test(NAME fzero_shim_ordinary_${mode} COMMAND fzero_shim_ordinary_tests ${mode})
        endforeach()
    endif()
    add_executable(fzero_hdma_tests tests/test_hdma.c)
    target_link_libraries(fzero_hdma_tests PRIVATE fzero_video)
    add_test(NAME fzero_hdma COMMAND fzero_hdma_tests)
    add_executable(fzero_video_tests tests/test_video.c)
    target_link_libraries(fzero_video_tests PRIVATE fzero_video)
    add_test(NAME fzero_video COMMAND fzero_video_tests)
    add_executable(fzero_panel_tests tests/test_panel.c src/fzero_hotkeys.c)
    target_link_libraries(fzero_panel_tests PRIVATE fzero_video)
    add_test(NAME fzero_panel COMMAND fzero_panel_tests)
    add_executable(fzero_triple_display_tests tests/test_triple_display.c)
    target_link_libraries(fzero_triple_display_tests PRIVATE fzero_video)
    add_test(NAME fzero_triple_display COMMAND fzero_triple_display_tests)
    add_executable(fzero_mode7_tests tests/test_mode7.c)
    target_link_libraries(fzero_mode7_tests PRIVATE fzero_video)
    add_test(NAME fzero_mode7 COMMAND fzero_mode7_tests)
    # Neither of these needs SDL, a ROM, or the generated sources: they are
    # the host-side logic behind the save-state menu and rewind bindings, so
    # they build and run in the ROM-free configuration too.
    add_executable(fzero_hotkeys_tests tests/test_hotkeys.c src/fzero_hotkeys.c)
    target_include_directories(fzero_hotkeys_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
    add_test(NAME fzero_hotkeys COMMAND fzero_hotkeys_tests)
    add_executable(fzero_state_mode_tests tests/test_state_mode.c src/fzero_state_mode.c)
    target_include_directories(fzero_state_mode_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
    add_test(NAME fzero_state_mode COMMAND fzero_state_mode_tests)
    add_executable(fzero_surface_trace_tests tests/test_surface_trace.c)
    target_include_directories(fzero_surface_trace_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
    add_test(NAME fzero_surface_trace COMMAND fzero_surface_trace_tests)
    if(NOT MSVC)
        target_compile_options(fzero_hotkeys_tests PRIVATE -Wall -Wextra -Werror)
        target_compile_options(fzero_state_mode_tests PRIVATE -Wall -Wextra -Werror)
    endif()
endif()
add_library(fzero_triple_geometry STATIC src/fzero_triple_geometry.c
    src/fzero_triple_ground.c)
target_include_directories(fzero_triple_geometry PUBLIC "${CMAKE_SOURCE_DIR}/src")
if(NOT MSVC)
    target_link_libraries(fzero_triple_geometry PUBLIC m)
endif()
if(BUILD_TESTING)
    add_executable(fzero_triple_geometry_tests tests/test_triple_geometry.c)
    target_link_libraries(fzero_triple_geometry_tests PRIVATE fzero_triple_geometry)
    add_test(NAME fzero_triple_geometry COMMAND fzero_triple_geometry_tests)
endif()
if(NOT FZERO_BUILD_GAME)
    if(BUILD_TESTING)
        add_executable(fzero_analog_tests tests/test_analog.c src/fzero_analog.c)
        target_include_directories(fzero_analog_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
        if(NOT MSVC)
            target_link_libraries(fzero_analog_tests PRIVATE m)
        endif()
        add_test(NAME fzero_analog COMMAND fzero_analog_tests)
        add_executable(fzero_playthrough_tests tests/test_playthrough.c src/fzero_playthrough.c)
        target_include_directories(fzero_playthrough_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
        add_test(NAME fzero_playthrough COMMAND fzero_playthrough_tests)
    endif()
    return()
endif()

# Both dependencies are pinned submodules of this repository (see
# .gitmodules); `bash tools/bootstrap.sh` initializes them. Point
# SNESRECOMP_ROOT at a worktree instead when developing framework changes
# alongside the game.
set(SNESRECOMP_ROOT "${CMAKE_SOURCE_DIR}/snesrecomp" CACHE PATH
    "Path to the snesrecomp checkout used by this game")
set(SNESRECOMP_SDL_BACKEND "SDL3" CACHE STRING
    "Desktop SDL backend (SDL3 or SDL2)")
set(RECOMP_UI_ENABLE_MODS ON CACHE BOOL "F-Zero built-in native presentation mod" FORCE)
include(${SNESRECOMP_ROOT}/runner/runner.cmake)
include(${RECOMP_UI_ROOT}/recomp_ui.cmake)
find_package(OpenGL REQUIRED)

add_executable(FZeroRenderCapture tools/render_capture.c src/fzero_renderer.c)
target_include_directories(FZeroRenderCapture PRIVATE ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
target_link_libraries(FZeroRenderCapture PRIVATE fzero_video fzero_triple_geometry)
# Include the renderer only in this offline tool so its course/VRAM sampling
# can be exercised without changing or linking the installed game host.
add_executable(FZeroTripleGroundCapture tools/triple_ground_capture.c)
target_include_directories(FZeroTripleGroundCapture PRIVATE ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
target_link_libraries(FZeroTripleGroundCapture PRIVATE fzero_video fzero_triple_geometry)
add_executable(FZeroTripleRuntimeCapture tools/triple_runtime_capture.c src/fzero_renderer.c)
target_include_directories(FZeroTripleRuntimeCapture PRIVATE ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
target_link_libraries(FZeroTripleRuntimeCapture PRIVATE fzero_video fzero_triple_geometry)
add_executable(FZeroRendererBenchmark tools/benchmark_renderer.c src/fzero_renderer.c)
target_include_directories(FZeroRendererBenchmark PRIVATE ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
target_link_libraries(FZeroRendererBenchmark PRIVATE fzero_video fzero_triple_geometry)
if(BUILD_TESTING)
    add_executable(fzero_renderer_tests tests/test_renderer.c src/fzero_renderer.c)
    add_executable(fzero_mods_tests tests/test_mods.c src/fzero_mods.c
        src/fzero_hotkeys.c src/fzero_analog.c)
    if(FZERO_DELUXE_GEN_DIR)
        target_compile_definitions(fzero_mods_tests PRIVATE FZERO_HAS_DELUXE=1)
    endif()
    target_include_directories(fzero_mods_tests PRIVATE "${RECOMP_UI_ROOT}/src" "${RECOMP_UI_ROOT}/src/common")
    target_link_libraries(fzero_mods_tests PRIVATE fzero_video)
    if(NOT MSVC)
        target_link_libraries(fzero_mods_tests PRIVATE m)
        target_compile_options(fzero_mods_tests PRIVATE -Wall -Wextra -Werror)
    endif()
    add_test(NAME fzero_mods COMMAND fzero_mods_tests)
    target_include_directories(fzero_renderer_tests PRIVATE ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
    target_link_libraries(fzero_renderer_tests PRIVATE fzero_video fzero_triple_geometry)
    if(NOT MSVC)
        target_compile_options(fzero_renderer_tests PRIVATE -Wall -Wextra -Werror)
    endif()
    add_test(NAME fzero_renderer COMMAND fzero_renderer_tests)
    add_executable(fzero_ground_colour_tests tests/test_ground_colour.c)
    target_include_directories(fzero_ground_colour_tests PRIVATE ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
    target_link_libraries(fzero_ground_colour_tests PRIVATE fzero_video fzero_triple_geometry)
    add_test(NAME fzero_ground_colour COMMAND fzero_ground_colour_tests)
endif()

# Release version stamp defaults to VERSION; override explicitly for local builds.
# NOTE: quote it when configuring - PowerShell rewrites an unquoted
# -DSNESRECOMP_BUILD_VERSION=0.1.0 into "0".
set(SNESRECOMP_BUILD_VERSION "${PROJECT_VERSION}" CACHE STRING
    "Version stamp baked into crash reports")

set(FZERO_GEN_DIR "${CMAKE_SOURCE_DIR}/src/gen" CACHE PATH
    "Directory containing the private generated F-Zero C sources")
file(GLOB FZERO_GEN_SOURCES CONFIGURE_DEPENDS "${FZERO_GEN_DIR}/*.c")
if(NOT FZERO_GEN_SOURCES)
    message(FATAL_ERROR
        "${FZERO_GEN_DIR} is empty - stage a verified fzero.sfc and run "
        "`bash tools/regen.sh` before building.")
endif()

# The headless host links no SDL at all, so anything in the runner that
# includes desktop/sdl_compat.h has to be held back from the shared source
# list and handed to the desktop host explicitly. keybinds.c and
# host_report.c were always in that set; snes_savestate_menu.c and
# snes_osd.c joined the runner later and pull SDL in for their keycodes and
# their tick clock.
list(REMOVE_ITEM SNESRECOMP_RUNNER_SOURCES
    "${SNESRECOMP_ROOT}/runner/src/keybinds.c"
    "${SNESRECOMP_ROOT}/runner/src/host_report.c"
    "${SNESRECOMP_ROOT}/runner/src/snes_savestate_menu.c"
    "${SNESRECOMP_ROOT}/runner/src/snes_osd.c")

set(FZERO_DELUXE_GEN_DIR "" CACHE PATH "Private generated BS Deluxe native sources")
set(FZERO_DELUXE_DATA_FILE "" CACHE FILEPATH "Imported BS Deluxe payload to embed")
if(FZERO_DELUXE_GEN_DIR)
    # The payload ships inside the executable: a download can never be missing
    # it and the game can never fail to launch over it. Default to the file the
    # importer writes beside the generated sources.
    if(NOT FZERO_DELUXE_DATA_FILE)
        get_filename_component(FZERO_DELUXE_WORK "${FZERO_DELUXE_GEN_DIR}" DIRECTORY)
        set(FZERO_DELUXE_DATA_FILE "${FZERO_DELUXE_WORK}/mods/bs-deluxe.dat")
    endif()
    if(NOT EXISTS "${FZERO_DELUXE_DATA_FILE}")
        message(FATAL_ERROR "BS Deluxe payload not found at ${FZERO_DELUXE_DATA_FILE}. "
                            "Run tools/regen_bs_deluxe.py, or set FZERO_DELUXE_DATA_FILE.")
    endif()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    set(FZERO_DELUXE_PAYLOAD_SOURCE "${CMAKE_BINARY_DIR}/generated/bs_deluxe_payload.c")
    add_custom_command(OUTPUT "${FZERO_DELUXE_PAYLOAD_SOURCE}"
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/embed_payload.py"
                "${FZERO_DELUXE_DATA_FILE}" "${FZERO_DELUXE_PAYLOAD_SOURCE}"
        DEPENDS "${FZERO_DELUXE_DATA_FILE}" "${CMAKE_SOURCE_DIR}/tools/embed_payload.py"
        COMMENT "Embedding BS Deluxe payload" VERBATIM)
    add_custom_target(fzero_deluxe_payload_source DEPENDS "${FZERO_DELUXE_PAYLOAD_SOURCE}")
    file(GLOB DELUXE_SOURCES CONFIGURE_DEPENDS "${FZERO_DELUXE_GEN_DIR}/*.c")
    if(NOT DELUXE_SOURCES OR NOT EXISTS "${FZERO_DELUXE_GEN_DIR}/deluxe_namespace.h")
        message(FATAL_ERROR "Generate and import BS Deluxe before enabling its native module")
    endif()
    add_library(fzero_deluxe_native OBJECT ${DELUXE_SOURCES})
    target_include_directories(fzero_deluxe_native PRIVATE
        "${CMAKE_SOURCE_DIR}/recomp" ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
    if(MSVC)
        target_compile_options(fzero_deluxe_native PRIVATE "/FI${FZERO_DELUXE_GEN_DIR}/deluxe_namespace.h")
    else()
        target_compile_options(fzero_deluxe_native PRIVATE -w -include "${FZERO_DELUXE_GEN_DIR}/deluxe_namespace.h")
    endif()
endif()

# The headless host keeps its own host_report stubs: it links no SDL, and
# host_report.c includes desktop/sdl_compat.h.
add_executable(FZeroSNESRecompHeadless
    ${SNESRECOMP_RUNNER_SOURCES}
    ${FZERO_GEN_SOURCES}
    src/headless_host.c
    src/headless_main.c
    src/fzero_playthrough.c
    src/fzero_ffb.cpp src/fzero_shared_owner.cpp
    src/fzero_output_latch.c
    src/fzero_hotkeys.c
    src/fzero_runtime.c)
target_compile_features(FZeroSNESRecompHeadless PRIVATE cxx_std_17) # shared owner's canonical controls parser

# The desktop host takes the REAL host_report.c (always-on breadcrumb ring,
# crash minidump writer, build-version stamp) and the REAL keybinds.c, so the
# launcher's Controls page and the game read the same keybinds.ini.
add_executable(FZeroSNESRecomp
    ${SNESRECOMP_RUNNER_SOURCES}
    "${SNESRECOMP_ROOT}/runner/src/host_report.c"
    "${SNESRECOMP_ROOT}/runner/src/keybinds.c"
    "${SNESRECOMP_ROOT}/runner/src/snes_savestate_menu.c"
    "${SNESRECOMP_ROOT}/runner/src/snes_osd.c"
    "${RECOMP_UI_ROOT}/src/third_party/gl_core_3_1.c"
    ${FZERO_GEN_SOURCES}
    src/sdl_main.c
    src/fzero_session_ui.c
    src/fzero_session_control.c
    src/fzero_diagnostics.c
    src/fzero_frame_stats.cpp
    src/fzero_setup.cpp
    src/fzero_mods.c
    src/fzero_hotkeys.c
    src/fzero_gamepad.c
    src/fzero_analog.c
    src/fzero_controls.cpp
    src/fzero_controls_sdl.c
    src/fzero_inject.cpp
    src/fzero_shim.cpp
    src/fzero_shim_map.c
    src/fzero_output_latch.c
    src/fzero_telemetry.cpp
    src/fzero_ffb.cpp src/fzero_shared_owner.cpp
    src/fzero_playthrough.c
    src/fzero_runtime.c)
target_compile_definitions(FZeroSNESRecomp PRIVATE FZERO_RENDER_TIMINGS=1)
target_compile_features(FZeroSNESRecomp PRIVATE cxx_std_17) # fzero_controls.cpp: the toolkit's C++17 header

function(configure_fzero_target target)
    target_sources(${target} PRIVATE src/fzero_deluxe.c src/fzero_state_mode.c src/fzero_msu.c)
    if(FZERO_DELUXE_GEN_DIR)
        target_sources(${target} PRIVATE $<TARGET_OBJECTS:fzero_deluxe_native>
                                         "${FZERO_DELUXE_PAYLOAD_SOURCE}")
        add_dependencies(${target} fzero_deluxe_payload_source)
        target_compile_definitions(${target} PRIVATE FZERO_HAS_DELUXE=1)
    endif()
    target_sources(${target} PRIVATE src/fzero_renderer.c)
    target_link_libraries(${target} PRIVATE fzero_video fzero_triple_geometry)
    target_include_directories(${target} PRIVATE
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/recomp"
        ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE
        SYSTEM_VOLUME_MIXER_AVAILABLE=0
        SNESRECOMP_TRACE=$<BOOL:${SNESRECOMP_ENABLE_TRACE}>
        SNESRECOMP_REVERSE_DEBUG=0)
    if(NOT MSVC)
        # The recompiled C is machine-generated; quiet its warnings and
        # tolerate the K&R-style implicit declarations clang rejects.
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-w;-Wno-implicit-function-declaration>)
        target_link_libraries(${target} PRIVATE m)
    endif()
    target_link_libraries(${target} PRIVATE ${SNESRECOMP_RUNNER_LIBRARIES})
endfunction()

configure_fzero_target(FZeroSNESRecompHeadless)
configure_fzero_target(FZeroSNESRecomp)
# Hosts include "desktop/sdl_compat.h" rather than <SDL.h> so both majors build.
snesrecomp_target_glsl_shader(FZeroSNESRecomp)
snesrecomp_target_sdl(FZeroSNESRecomp)

if(BUILD_TESTING AND SNESRECOMP_SDL_BACKEND STREQUAL "SDL3")
    add_executable(fzero_gamepad_tests tests/test_gamepad.c src/fzero_gamepad.c src/fzero_hotkeys.c src/fzero_inject.cpp src/fzero_controls.cpp src/fzero_output_latch.c)
    target_sources(fzero_gamepad_tests PRIVATE src/fzero_analog.c)
    target_compile_features(fzero_gamepad_tests PRIVATE cxx_std_17)   # fzero_inject.cpp, fzero_controls.cpp
    target_compile_definitions(fzero_gamepad_tests PRIVATE FZERO_TEST_IDENTITY_PATH)
    target_include_directories(fzero_gamepad_tests PRIVATE "${CMAKE_SOURCE_DIR}/src" "${RECOMP_UI_ROOT}/src/common" ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
    snesrecomp_target_sdl(fzero_gamepad_tests)
    add_test(NAME fzero_gamepad COMMAND fzero_gamepad_tests)

    # STD-033 rig-profile controls: [Controls] -> [Controller.<GUID>] at startup, read back by the game's reader.
    add_executable(fzero_controls_tests tests/test_controls.c src/fzero_controls.cpp src/fzero_controls_sdl.c src/fzero_inject.cpp
        src/fzero_gamepad.c src/fzero_analog.c src/fzero_hotkeys.c src/fzero_output_latch.c)
    target_include_directories(fzero_controls_tests PRIVATE "${CMAKE_SOURCE_DIR}/src" "${RECOMP_UI_ROOT}/src/common" ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
    target_compile_features(fzero_controls_tests PRIVATE cxx_std_17)
    snesrecomp_target_sdl(fzero_controls_tests)
    if(NOT MSVC)
        target_link_libraries(fzero_gamepad_tests PRIVATE m)
        target_link_libraries(fzero_controls_tests PRIVATE m)
    endif()
    add_test(NAME fzero_controls COMMAND fzero_controls_tests)

    add_executable(fzero_analog_tests tests/test_analog.c src/fzero_analog.c)
    target_include_directories(fzero_analog_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
    if(NOT MSVC)
        target_link_libraries(fzero_analog_tests PRIVATE m)
    endif()
    add_test(NAME fzero_analog COMMAND fzero_analog_tests)
endif()
target_link_libraries(FZeroSNESRecomp PRIVATE OpenGL::GL)
if(WIN32)
    target_link_libraries(FZeroSNESRecomp PRIVATE ws2_32 bcrypt)
    if(FZERO_SETUP_BUNDLE)
        add_custom_command(TARGET FZeroSNESRecomp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${FZERO_SETUP_BUNDLE}"
                "$<TARGET_FILE_DIR:FZeroSNESRecomp>/controls-setup"
            COMMENT "Staging qualified private controls setup bundle" VERBATIM)
    endif()
    # A native-only toolkit update must refresh the runtime beside an otherwise
    # unchanged executable; POST_BUILD alone would leave the old DLL staged.
    set(_fzero_wheel_runtime "${CMAKE_SOURCE_DIR}/lib/toolkit/native/WheelFfb.dll")
    if(FZERO_SHARED_FORCE_DEPENDENCIES)
        set(_fzero_wheel_runtime "${FZERO_SHARED_FORCE_WHEEL_DLL}")
    endif()
    set_property(TARGET FZeroSNESRecomp APPEND PROPERTY LINK_DEPENDS "${_fzero_wheel_runtime}")
    add_custom_command(TARGET FZeroSNESRecomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_fzero_wheel_runtime}"
            "$<TARGET_FILE_DIR:FZeroSNESRecomp>/WheelFfb.dll"
        COMMENT "Staging WheelFfb runtime"
        VERBATIM)
    # Provisional private dependency, not a default player-package payload.
    set(FZERO_SHIM_DLL "" CACHE FILEPATH "Reviewed ABI2 DLL for private shim diagnostics only")
    set(_fzero_shim_runtime "${FZERO_SHIM_DLL}")
    if(FZERO_SHARED_FORCE_DEPENDENCIES)
        set(_fzero_shim_runtime "${FZERO_SHARED_FORCE_SHIM_DLL}")
    endif()
    if(_fzero_shim_runtime AND NOT FZERO_SHARED_FORCE_DEPENDENCIES)
        file(SHA256 "${_fzero_shim_runtime}" FZERO_SHIM_SHA)
        if(NOT FZERO_SHIM_SHA STREQUAL "bf6455e63696a93b37c19dabf09b1e0606f26b0534bce828417b1df602dd8a7a")
            message(FATAL_ERROR "FZERO_SHIM_DLL is not the reviewed ea568c2 extras-capable private runtime")
        endif()
    endif()
    if(_fzero_shim_runtime)
        set_property(TARGET FZeroSNESRecomp APPEND PROPERTY LINK_DEPENDS "${_fzero_shim_runtime}")
        add_custom_command(TARGET FZeroSNESRecomp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_fzero_shim_runtime}"
                "$<TARGET_FILE_DIR:FZeroSNESRecomp>/dbce_shim.dll"
            COMMENT "Staging diagnostic-only shared input shim" VERBATIM)
    endif()
endif()

if(BUILD_TESTING)
    add_executable(fzero_playthrough_tests tests/test_playthrough.c src/fzero_playthrough.c)
    target_include_directories(fzero_playthrough_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
    add_test(NAME fzero_playthrough COMMAND fzero_playthrough_tests)
    add_executable(fzero_telemetry_tests tests/test_telemetry.cpp
        src/fzero_telemetry.cpp src/fzero_hotkeys.c)
    target_include_directories(fzero_telemetry_tests PRIVATE
        "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/lib/toolkit/include")
    if(WIN32)
        target_link_libraries(fzero_telemetry_tests PRIVATE ws2_32)
    endif()
    add_test(NAME fzero_telemetry COMMAND fzero_telemetry_tests)

    add_executable(fzero_ffb_tests tests/test_ffb.cpp src/fzero_ffb.cpp
        src/fzero_hotkeys.c)
    target_include_directories(fzero_ffb_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
    # Offline means no optional native loader or device discovery, even when a
    # real WheelFfb.dll is beside the test. Hardware lifecycle has a fake suite.
    target_compile_definitions(fzero_ffb_tests PRIVATE FZERO_FFB_MODEL_ONLY)
    add_test(NAME fzero_ffb COMMAND fzero_ffb_tests)
    add_executable(fzero_force_trace tools/force_model_trace.cpp src/fzero_ffb.cpp src/fzero_hotkeys.c)
    target_include_directories(fzero_force_trace PRIVATE "${CMAKE_SOURCE_DIR}/src")
    target_compile_definitions(fzero_force_trace PRIVATE FZERO_FFB_MODEL_ONLY)
    if(WIN32)
        # Fake Win32 loader/exports: no real DLL or device is accessed.
        add_executable(fzero_ffb_lifecycle_tests tests/test_ffb_lifecycle.cpp src/fzero_shared_owner.cpp
            src/fzero_hotkeys.c src/fzero_output_latch.c src/fzero_inject.cpp src/fzero_controls.cpp)
        target_compile_features(fzero_ffb_lifecycle_tests PRIVATE cxx_std_17)   # fzero_inject.cpp, fzero_controls.cpp
        add_test(NAME fzero_ffb_lifecycle COMMAND fzero_ffb_lifecycle_tests)
        # The no-force latch is process-wide: each order is its own process.
        add_test(NAME fzero_ffb_injection_latch COMMAND fzero_ffb_lifecycle_tests --injection-latch)
        add_test(NAME fzero_ffb_force_first COMMAND fzero_ffb_lifecycle_tests --force-first)
        add_test(NAME fzero_ffb_checked_refusal COMMAND fzero_ffb_lifecycle_tests --checked-refusal)
    endif()
endif()
recomp_target_launcher_ui(FZeroSNESRecomp CONSOLE snes)
# Identify dependencies as well as the game in player performance reports.
find_package(Git QUIET)
function(fzero_revision directory output)
    set(revision "unknown")
    if(GIT_FOUND)
        execute_process(COMMAND "${GIT_EXECUTABLE}" describe --always --dirty --abbrev=12
            WORKING_DIRECTORY "${directory}" OUTPUT_VARIABLE revision
            RESULT_VARIABLE revision_result OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if(NOT revision_result EQUAL 0)
            set(revision "unknown")
        endif()
    endif()
    set(${output} "${revision}" PARENT_SCOPE)
endfunction()
fzero_revision("${CMAKE_CURRENT_SOURCE_DIR}" FZERO_SOURCE_REVISION)
fzero_revision("${SNESRECOMP_ROOT}" FZERO_FRAMEWORK_REVISION)
fzero_revision("${RECOMP_UI_ROOT}" FZERO_UI_REVISION)
configure_file(src/fzero_build.h.in generated/fzero_build.h @ONLY)
target_include_directories(FZeroSNESRecomp PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/img/boxart.tga")
add_custom_command(TARGET FZeroSNESRecomp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/img/boxart.tga"
        "$<TARGET_FILE_DIR:FZeroSNESRecomp>/assets/img/boxart.tga"
    VERBATIM)
endif()
add_custom_command(TARGET FZeroSNESRecomp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
        "$<TARGET_FILE_DIR:FZeroSNESRecomp>/assets/shaders"
    COMMENT "Staging F-Zero shader presets"
    VERBATIM)

if(SNESRECOMP_BUILD_VERSION)
    target_compile_definitions(FZeroSNESRecomp PRIVATE
        SNESRECOMP_BUILD_VERSION="${SNESRECOMP_BUILD_VERSION}")
endif()

if(BUILD_TESTING AND FZERO_DELUXE_GEN_DIR)
    # Locks the contract that matters for a download: the payload is in the
    # binary, a file beside it is only an override, and nothing here can
    # stop the game starting.
    add_executable(fzero_deluxe_tests tests/test_deluxe.c tests/deluxe_stubs.c
        src/fzero_deluxe.c "${SNESRECOMP_ROOT}/runner/src/sha256.c"
        "${FZERO_DELUXE_PAYLOAD_SOURCE}")
    add_dependencies(fzero_deluxe_tests fzero_deluxe_payload_source)
    target_compile_definitions(fzero_deluxe_tests PRIVATE FZERO_HAS_DELUXE=1)
    target_include_directories(fzero_deluxe_tests PRIVATE
        "${CMAKE_SOURCE_DIR}/src" ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
    if(NOT MSVC)
        target_compile_options(fzero_deluxe_tests PRIVATE -Wall -Wextra -Werror)
    endif()
    add_test(NAME fzero_deluxe COMMAND fzero_deluxe_tests
             WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
