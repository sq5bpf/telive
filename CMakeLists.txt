cmake_minimum_required(VERSION 3.7)
project(telive)

find_package(LibXml2 REQUIRED)
if (LIBXML2_FOUND)
    include_directories(${LIBXML2_INCLUDE_DIR})
endif (LIBXML2_FOUND)

find_package(Curses REQUIRED)
if (CURSES_FOUND)
    include_directories(${CURSES_INCLUDE_DIR})
endif(CURSES_FOUND)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        telive.c
        telive.h
        telive_receiver.c
        telive_receiver.h
        telive_util.c
        telive_util.h)

add_executable(telive ${SOURCE_FILES})
target_link_libraries(telive
        ${LIBXML2_LIBRARIES}
        ${CURSES_LIBRARIES})