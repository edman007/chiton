ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -std=gnu++11 -Wall
PRGM = chiton

bin_PROGRAMS = $(PRGM)
chiton_SOURCES = main.cpp config.cpp config_parser.cpp database.cpp mariadb.cpp  mysql.cpp util.cpp mariadb_result.cpp
chiton_CXXFLAGS = $(AM_CXXFLAGS)
# chiton_LIBS = $(AM_LIBS) $(MARIADB_LIBS) -lmysql
