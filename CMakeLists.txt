cmake_minimum_required(VERSION 3.5)

project(engine-psp)

file(GLOB FLECS_SRC ${CMAKE_CURRENT_LIST_DIR}/deps/flecs/*.c)
add_library(flecs ${FLECS_SRC})
target_include_directories(flecs PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/deps/flecs/>)

add_executable(${PROJECT_NAME}
  src/main.c)

include(FindPkgConfig)

pkg_search_module(SDL2 REQUIRED sdl2)

target_include_directories(${PROJECT_NAME} PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/deps/flecs/>
  ${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${SDL2_LIBRARIES}
  flecs)

if(PSP)
  add_compile_definitions(
    -DFLECS_NO_CPP
    -DFLECS_NO_HTTP
    -DFLECS_NO_REST
    -D__PSP__)
  
  create_pbp_file(
    TARGET ${PROJECT_NAME}
    ICON_PATH NULL
    BACKGROUND_PATH NULL
    PREVIEW_PATH NULL
    TITLE "Luksamuk PSP Engine"
    VERSION 01.00)
endif()


set_property(TARGET flecs PROPERTY C_STANDARD 99)

