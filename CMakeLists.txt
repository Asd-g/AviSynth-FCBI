cmake_minimum_required(VERSION 3.16)

project(libfcbi LANGUAGES CXX)

option(BUILD_AVS_LIB "Build library for AviSynth+" ON)
option(BUILD_VS_LIB "Build library for VapourSynth" ON)

message(STATUS "Build library for AviSynth - ${BUILD_AVS_LIB}")
message(STATUS "Build library for VapourSynth - ${BUILD_VS_LIB}")

set (sources
    src/fcbi_c.cpp
    src/fcbi_sse2.cpp
    src/VCL2/instrset_detect.cpp
)

if (BUILD_AVS_LIB)
    set (sources
        ${sources}
        src/fcbi_avs.cpp
    )
endif()

if (BUILD_VS_LIB)
    set (sources
        ${sources}
        src/fcbi_vs.cpp
    )
endif()

add_library(fcbi SHARED ${sources})

target_include_directories(fcbi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (BUILD_AVS_LIB)
    target_include_directories(fcbi PRIVATE /usr/local/include/avisynth)
endif()

if (BUILD_VS_LIB)
    target_include_directories(fcbi PRIVATE /usr/local/include/vapoursynth)
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
if (build_type STREQUAL debug)
    target_compile_definitions(fcbi PRIVATE DEBUG_BUILD)
else (build_type STREQUAL release)
    target_compile_definitions(fcbi PRIVATE RELEASE_BUILD)
endif ()

message(STATUS "Build type - ${CMAKE_BUILD_TYPE}")

target_compile_features(fcbi PRIVATE cxx_std_17)

set_source_files_properties(src/fcbi_sse2.cpp PROPERTIES COMPILE_OPTIONS "-mfpmath=sse;-msse2")

find_package (Git)

if (GIT_FOUND)
    execute_process (COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        OUTPUT_VARIABLE ver
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set_target_properties(fcbi PROPERTIES OUTPUT_NAME "fcbi.${ver}")
else ()
    message (STATUS "GIT not found")
endif ()

include(GNUInstallDirs)

if (BUILD_AVS_LIB)
    INSTALL(TARGETS fcbi LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/avisynth")
endif()

if (BUILD_VS_LIB)
    INSTALL(TARGETS fcbi LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/vapoursynth")
endif()

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
