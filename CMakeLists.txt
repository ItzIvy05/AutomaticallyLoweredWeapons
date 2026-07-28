cmake_minimum_required(VERSION 3.21)

set(AUTHOR_NAME "Ivy")
set(PRODUCT_NAME "AutomaticallyLoweredWeapons")
set(PLUGIN_OUTPUT_NAME "AutomaticallyLoweredWeapons")

project(${PRODUCT_NAME} VERSION 2.0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

set(F4_RUNTIME "AE" CACHE STRING "Target Fallout 4 runtime era: PRENG or AE")
set_property(CACHE F4_RUNTIME PROPERTY STRINGS PRENG AE)

set(TOOLS_ROOT "D:/Fallout DLL Mods/tools")
set(MODS_OUTPUT_ROOT "D:/Fallout DLL Mods/Output")

if(DEFINED ENV{FALLOUT_TOOLS_FOLDER})
    set(TOOLS_ROOT "$ENV{FALLOUT_TOOLS_FOLDER}")
endif()

if(DEFINED ENV{FALLOUT_MODS_FOLDER})
    set(MODS_OUTPUT_ROOT "$ENV{FALLOUT_MODS_FOLDER}")
endif()

if(F4_RUNTIME STREQUAL "PRENG")
    set(COMMONLIB_F4_FOLDER "${TOOLS_ROOT}/CommonLibF4-PreNG")
    set(RUNTIME_SUFFIX "PreNG")
elseif(F4_RUNTIME STREQUAL "AE")
    set(COMMONLIB_F4_FOLDER "${TOOLS_ROOT}/CommonLibF4-AE")
    set(RUNTIME_SUFFIX "AE")
else()
    message(FATAL_ERROR "F4_RUNTIME must be PRENG or AE, got '${F4_RUNTIME}'")
endif()

if(DEFINED ENV{COMMONLIB_F4_${F4_RUNTIME}_FOLDER})
    set(COMMONLIB_F4_FOLDER "$ENV{COMMONLIB_F4_${F4_RUNTIME}_FOLDER}")
endif()

if(NOT EXISTS "${COMMONLIB_F4_FOLDER}/CommonLibF4/CMakeLists.txt")
    message(FATAL_ERROR "CommonLibF4 not found at '${COMMONLIB_F4_FOLDER}'")
endif()

message(STATUS "F4_RUNTIME: ${F4_RUNTIME}")
message(STATUS "CommonLibF4: ${COMMONLIB_F4_FOLDER}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    @ONLY)

add_subdirectory("${COMMONLIB_F4_FOLDER}/CommonLibF4" ${CMAKE_CURRENT_BINARY_DIR}/_deps/clibf4-build)

find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h" REQUIRED)

file(GLOB_RECURSE headers ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${headers} ${sources})

add_library(
    ${PROJECT_NAME}
    SHARED
    ${headers}
    ${sources}
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
    F4_RUNTIME_${F4_RUNTIME}=1)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SIMPLEINI_INCLUDE_DIRS})

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    CommonLibF4::CommonLibF4)

target_precompile_headers(${PROJECT_NAME} PRIVATE include/PCH.h)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    OUTPUT_NAME "${PLUGIN_OUTPUT_NAME}"
    SUFFIX ".dll")

set(MOD_ROOT "${MODS_OUTPUT_ROOT}/${PRODUCT_NAME}-${RUNTIME_SUFFIX}")
set(DEPLOY_DIR "${MOD_ROOT}/F4SE/Plugins")

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DEPLOY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/package/${PLUGIN_OUTPUT_NAME}.ini" "${DEPLOY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/package/AutoLoweredWeapons.esp" "${MOD_ROOT}"
    VERBATIM)
