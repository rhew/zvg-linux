cmake_minimum_required(VERSION 2.8)

project(zvg C)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)

find_package(Threads REQUIRED)

include_directories(inc)

set(LIBZVG_SOURCES shared/timer.c shared/zvgban.c shared/zvgenc.c shared/zvgerror.c shared/zvgframe.c shared/zvgport.c shared/wrappers.c)
add_library(zvg SHARED ${LIBZVG_SOURCES} inc)

add_executable(bin/zvgTweak zvgtweak/zvgtweak.c)
target_link_libraries(bin/zvgTweak zvg ${CURSES_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

add_executable(bin/frmDemo frmdemo/frmdemo.c)
target_link_libraries(bin/frmDemo zvg ${CURSES_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
