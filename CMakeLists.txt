cmake_minimum_required(VERSION 2.8)

project(zvg C)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)

find_package(Threads REQUIRED)

include_directories(inc)
add_subdirectory(shared)

add_executable(bin/zvgTweak zvgtweak/zvgtweak.c)
target_link_libraries(bin/zvgTweak shared ${CURSES_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

add_executable(bin/frmDemo frmdemo/frmdemo.c)
target_link_libraries(bin/frmDemo shared ${CURSES_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
