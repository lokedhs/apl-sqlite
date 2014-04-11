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

#include "SqliteArgListBuilder.hh"

#include <string.h>

SqliteArgListBuilder::~SqliteArgListBuilder()
{
    for( vector<SqliteBindArg *>::iterator i = args.begin() ; i != args.end() ; i++ ) {
        delete *i;
    }
}

void SqliteArgListBuilder::bind_args( sqlite3_stmt *statement )
{
    int pos = 1;
    for( vector<SqliteBindArg *>::iterator i = args.begin() ; i != args.end() ; i++ ) {
        (*i)->bind( statement, pos++ );
    }    
}

static void free_text_arg( void *arg )
{
    free( arg );
}

template<>
void SqliteBindArgBind<string>::bind( sqlite3_stmt *statement, int pos )
{
    char *text = strdup( arg.c_str() );
    if( text == NULL ) {
        CERR << "Failed to allocate memory for bind arg" << endl;
        abort();
    }
    sqlite3_bind_text( statement, pos, text, -1, free_text_arg );
}

template<>
void SqliteBindArgBind<long>::bind( sqlite3_stmt *statement, int pos )
{
    sqlite3_bind_int64( statement, pos, arg );
}

template<>
void SqliteBindArgBind<double>::bind( sqlite3_stmt *statement, int pos )
{
    sqlite3_bind_double( statement, pos, arg );
}

void SqliteBindArgNull::bind( sqlite3_stmt *statement, int pos )
{
    sqlite3_bind_null( statement, pos );
}

void SqliteArgListBuilder::append_string( const string &arg )
{
    args.push_back( new SqliteBindArgBind<string>( arg ) );
}

void SqliteArgListBuilder::append_long( long arg )
{
    args.push_back( new SqliteBindArgBind<long>( arg ) );
}

void SqliteArgListBuilder::append_double( double arg )
{
    args.push_back( new SqliteBindArgBind<double>( arg ) );
}

void SqliteArgListBuilder::append_null( void )
{
    args.push_back( new SqliteBindArgNull() );
}
