cmake_minimum_required(VERSION 3.31)
project(gameOfLife C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

link_directories(${CMAKE_SOURCE_DIR}/externals)
include_directories(${CMAKE_SOURCE_DIR}/externals)


add_executable(gpu_gol
        gpu_gol.c
)


if (EMSCRIPTEN)
include_directories(${CMAKE_SOURCE_DIR}/externals)
add_executable(gameOfLife
        web_gol.c
        #single_thread_gol.c
)
## Multithreaded version for WebAssembly
set(CMAKE_EXECUTABLE_SUFFIX ".html")
string(APPEND LINK_FLAGS "-Os -Wall ")
string(APPEND LINK_FLAGS "${CMAKE_SOURCE_DIR}/externals/libraylib_web.a ")
string(APPEND LINK_FLAGS "-I ${CMAKE_SOURCE_DIR}/externals ")
string(APPEND LINK_FLAGS "-s USE_GLFW=3 ")
string(APPEND LINK_FLAGS "-sASYNCIFY ")
string(APPEND LINK_FLAGS "-pthread ")
#string(APPEND LINK_FLAGS "-sSINGLE_FILE ")
string(APPEND LINK_FLAGS "-DPLATFORM_WEB ")
string(APPEND LINK_FLAGS "--shell-file ${CMAKE_SOURCE_DIR}/template.html ")
set_target_properties(gameOfLife PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
target_compile_definitions(gameOfLife PRIVATE PLATFORM_WEB)
##/ Single thread GPU version for WebAssembly
set(CMAKE_EXECUTABLE_SUFFIX ".html")
string(APPEND LINK_FLAGS2 "-Os -Wall ")
string(APPEND LINK_FLAGS2 "${CMAKE_SOURCE_DIR}/externals/libraylib_web.a ")
string(APPEND LINK_FLAGS2 "-I ${CMAKE_SOURCE_DIR}/externals ")
string(APPEND LINK_FLAGS2 "-s USE_GLFW=3 ")
string(APPEND LINK_FLAGS2 "-s USE_WEBGL2=1 ")
string(APPEND LINK_FLAGS2 "-sASYNCIFY ")
#string(APPEND LINK_FLAGS "-sSINGLE_FILE ")
string(APPEND LINK_FLAGS2 "-DPLATFORM_WEB ")
string(APPEND LINK_FLAGS2 "--shell-file ${CMAKE_SOURCE_DIR}/template.html ")
string(APPEND LINK_FLAGS2 "--embed-file ${CMAKE_SOURCE_DIR}/es_gol.fs@/es_gol.fs --embed-file ${CMAKE_SOURCE_DIR}/es_draw.fs@/es_draw.fs --embed-file ${CMAKE_SOURCE_DIR}/es_web.vs@/es_web.vs ")
set_target_properties(gpu_gol PROPERTIES LINK_FLAGS "${LINK_FLAGS2}")
target_compile_definitions(gpu_gol PRIVATE PLATFORM_WEB)
else()
find_package(Threads REQUIRED)
  # CPU (multithread) version with openGL
add_executable(gameOfLife
        game_of_life.c
        externals/glad.c
)
if (WIN32)
target_link_libraries(gpu_gol PRIVATE ${CMAKE_SOURCE_DIR}/externals/libraylib.a -lopengl32 -lgdi32 -lwinmm )
target_link_libraries(gameOfLife glfw3 opengl32 Threads::Threads)
endif(WIN32)
if (UNIX)
  target_link_libraries(gpu_gol PRIVATE ${CMAKE_SOURCE_DIR}/externals/libraylib_linux.a m z dl pthread rt X11)
  target_link_libraries(gameOfLife glfw m GL Threads::Threads)
endif(UNIX)

endif()
