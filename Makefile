#    This file is part of GNU APL, a free implementation of the
#    ISO/IEC Standard 13751, "Programming Language APL, Extended"
#
#    Copyright (C) 2014  Elias Mårtenson
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

APL_DIST = $(HOME)/src/apl
CXX = c++
CXXFLAGS = -Wall -Wno-sign-compare -fPIC -g -I$(APL_DIST)/src
LIBS = -lsqlite3

OBJS = apl-sqlite.o Sqlite3Connection.o ResultValue.o SqliteArgListBuilder.o

UNAME = $(shell uname)
ifeq ($(UNAME),Darwin)
SHARED_FLAGS = -dynamiclib -Wl,-undefined,dynamic_lookup
LIBRARY_EXT = dylib
else
SHARED_FLAGS = -shared
LIBRARY_EXT = so
endif
LIBNAME = lib_sqlite.$(LIBRARY_EXT)

all:		$(LIBNAME)

$(LIBNAME):	$(OBJS)
		$(CC) $(SHARED_FLAGS) $(CXXFLAGS) $(OBJS) -o $(LIBNAME) $(LIBS)

clean:
		rm -f $(OBJS) $(LIBNAME)
