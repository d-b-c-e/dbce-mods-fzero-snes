cmake_minimum_required(VERSION 3.24)
project(fzero_virtual_consumer_compile NONE)
if(CMAKE_CONFIGURATION_TYPES OR NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  message(FATAL_ERROR "The pinned consumer requires a single-config Release build")
endif()
if(NOT DEFINED SDL3_DIR OR NOT DEFINED RUNNER_ROOT OR NOT DEFINED RECOMP_UI_ROOT)
  message(FATAL_ERROR "Pass SDL3_DIR from the isolated build, RUNNER_ROOT and RECOMP_UI_ROOT")
endif()
find_package(SDL3 3.4.10 EXACT CONFIG REQUIRED PATHS "${SDL3_DIR}" NO_DEFAULT_PATH)
get_filename_component(ISOLATED_ROOT "${SDL3_DIR}/.." ABSOLUTE)
include("${CMAKE_CURRENT_LIST_DIR}/Admit.cmake")
enable_language(C CXX)
file(WRITE "${CMAKE_BINARY_DIR}/consumer-link-provenance.txt" "runtimeAllowed=false\narchive=${RESOLVED_ARCHIVE}\nsha256=${ACTUAL_HASH}\nconfigSHA256=${ACTUAL_CONFIG}\n")
get_filename_component(FZERO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_executable(fzero_virtual_consumer
  "${FZERO_ROOT}/tests/test_gamepad.c"
  "${FZERO_ROOT}/src/fzero_gamepad.c"
  "${FZERO_ROOT}/src/fzero_hotkeys.c"
  "${FZERO_ROOT}/src/fzero_inject.cpp"
  "${FZERO_ROOT}/src/fzero_controls.cpp"
  "${FZERO_ROOT}/src/fzero_output_latch.c"
  "${FZERO_ROOT}/src/fzero_analog.c")
target_compile_features(fzero_virtual_consumer PRIVATE c_std_11 cxx_std_17)
target_compile_definitions(fzero_virtual_consumer PRIVATE
  SNESRECOMP_SDL3=1 LNG_SDL3=1 FZERO_TEST_IDENTITY_PATH)
target_include_directories(fzero_virtual_consumer PRIVATE
  "${FZERO_ROOT}/src" "${RUNNER_ROOT}/src" "${RECOMP_UI_ROOT}/src/common")
target_link_libraries(fzero_virtual_consumer PRIVATE SDL3::SDL3-static)
# The shared SDL compatibility header includes SDL_main.h in the reader too.
# Emit SDL's Windows entry point only in the fixture's main translation unit.
set_source_files_properties("${FZERO_ROOT}/src/fzero_gamepad.c"
  PROPERTIES COMPILE_DEFINITIONS SDL_MAIN_NOIMPL)
# Intentionally no add_test or runtime hook: compile/link only.

# Recheck on every build, including builds that do not rerun configuration.
add_custom_target(admit_isolated_sdl
  COMMAND "${CMAKE_COMMAND}" "-DSDL3_DIR=${SDL3_DIR}" "-DRESOLVED_ARCHIVE=${RESOLVED_ARCHIVE}"
          -P "${CMAKE_CURRENT_LIST_DIR}/Admit.cmake"
  VERBATIM)
add_dependencies(fzero_virtual_consumer admit_isolated_sdl)
