AM_CFLAGS = $(MORE_CFLAGS)
AM_CPPFLAGS = $(MORE_CPPFLAGS)
AM_LDFLAGS = $(MORE_LDFLAGS)

dist_man_MANS = netlogging.8

## TODO:5000 When you want to add more files, add them below.
## TODO:5000 Automake will find dependencies by itself. Run
## TODO:5000 ./autogen.sh after modifying this file.

lib_LTLIBRARIES = libnetlogging.la
libnetlogging_la_SOURCES  = netlogging.c
libnetlogging_la_CFLAGS   = $(AM_CFLAGS)
libnetlogging_la_LDFLAGS  = $(AM_LDFLAGS)

## TODO:2004 Each time you have used `PKG_CHECK_MODULES` macro
## TODO:2004 in `configure.ac`, you get two variables that
## TODO:2004 you can substitute like above.
