cmake_minimum_required(VERSION 3.5)

project(engine-psp)

file(GLOB FLECS_SRC ${CMAKE_CURRENT_LIST_DIR}/deps/flecs/*.c)
add_library(flecs ${FLECS_SRC})
target_include_directories(flecs PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/deps/flecs/>)

file(GLOB ENGINE_SRC ${CMAKE_CURRENT_LIST_DIR}/src/*.c)

add_executable(${PROJECT_NAME} ${ENGINE_SRC})

include(FindPkgConfig)

target_include_directories(${PROJECT_NAME} PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/deps/flecs/>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  pspdisplay
  pspdebug
  pspgu
  pspge
  pspctrl
  pspaudio
  pspmp3
  psputility
  flecs
)

file(GLOB ASSETS ${CMAKE_CURRENT_LIST_DIR}/assets/*)
file(COPY ${ASSETS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/assets)

if(PSP)
  add_compile_definitions(
    -DFLECS_NO_CPP
    -DFLECS_NO_HTTP
    -DFLECS_NO_REST
    -D__PSP__)
  
  create_pbp_file(
    TARGET ${PROJECT_NAME}
    ICON_PATH "${CMAKE_CURRENT_LIST_DIR}/preview/ICON0.png"
    BACKGROUND_PATH "${CMAKE_CURRENT_LIST_DIR}/preview/PIC1.png"
    PREVIEW_PATH NULL
    TITLE "Luksamuk PSP Engine"
    VERSION 01.00)
endif()

set_property(TARGET flecs PROPERTY C_STANDARD 99)

