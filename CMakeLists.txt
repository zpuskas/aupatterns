CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(aupatterns C)

ADD_DEFINITIONS(-g -ggdb -pedantic -fsigned-char -freg-struct-return -Wall -W -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Winline -Werror)

ADD_SUBDIRECTORY(src bin)

