/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2014  Elias Mårtenson

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SQLITE_ARG_LIST_BUILDER_HH
#define SQLITE_ARG_LIST_BUILDER_HH

#include "apl-sqlite.hh"
#include "Sqlite3Connection.hh"
#include "ArgListBuilder.hh"

class SqliteBindArg {
public:
    virtual ~SqliteBindArg() {}
    virtual void bind( sqlite3_stmt *statement, int pos ) = 0;
};

template<class T>
class SqliteBindArgBind : public SqliteBindArg {
public:
    SqliteBindArgBind( const T &arg_in ) : arg( arg_in ) {}
    virtual void bind( sqlite3_stmt *statement, int pos );

private:
    T arg;
};

class SqliteBindArgNull : public SqliteBindArg {
public:
    virtual void bind( sqlite3_stmt *statement, int pos );    
};

class SqliteArgListBuilder : public ArgListBuilder {
public:
    SqliteArgListBuilder() {}
    virtual ~SqliteArgListBuilder();
    void append_string( const string &arg );
    void append_long( long arg );
    void append_double( double arg );
    void append_null( void );
    void bind_args( sqlite3_stmt *statement );

private:
    vector<SqliteBindArg *> args;
};

#endif
