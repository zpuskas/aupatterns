CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(aupatterns C)

SET(CMAKE_C_FLAGS_RELEASE "-O3")
SET(CMAKE_C_FLAGS_DEBUG "-g -ggdb")

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release")    
ENDIF(NOT CMAKE_BUILD_TYPE)

ADD_DEFINITIONS(-pedantic -fsigned-char -freg-struct-return -Wall -W -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Winline -Werror)

ADD_SUBDIRECTORY(src bin)

