cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MINGW)
    message(NOTICE "MINGW detected, adding coroutine support")
    add_compile_options(-fcoroutines)
elseif(MSVC)
    message(NOTICE "MSVC detected, adding coroutine support")
    add_compile_options(/await)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(NOTICE "Clang detected, adding coroutine support")
    add_compile_options(-fcoroutines-ts -stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif()

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)

FetchContent_Declare(cppcoro
    GIT_REPOSITORY https://github.com/shenli99/cppcoro.git
    GIT_TAG 26.1.9
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)


FetchContent_MakeAvailable(SFML cppcoro)

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" _RESOURCE_DIR_PATH)
string(REPLACE "\\" "/" _RESOURCE_DIR_PATH "${_RESOURCE_DIR_PATH}")

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(main ${SOURCES})
target_include_directories(main PRIVATE
    src/include
)

target_compile_definitions(main PRIVATE RESOURCE_DIR=\"${_RESOURCE_DIR_PATH}\")

target_compile_features(main PRIVATE cxx_std_20)
target_link_libraries(main PRIVATE
    SFML::Graphics
    SFML::Window
    cppcoro
)

add_custom_command(TARGET main POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/fonts/yahei.ttf"
    $<TARGET_FILE_DIR:main>/yahei.ttf
  COMMENT "Copy fonts/yahei.ttf -> ${TARGET_FILE_DIR:main}/yahei.ttf"
)

add_custom_command(TARGET TARGET_NAME POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/fonts"
        "<TARGET_FILE_DIR:TARGET_NAME>/"
        )

# set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)
