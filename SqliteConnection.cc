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

#include "apl-sqlite.hh"
#include "SqliteConnection.hh"
#include "ResultValue.hh"
#include "SqliteArgListBuilder.hh"

void SqliteConnection::raise_sqlite_error( const string &message )
{
    stringstream out;
    out << message << ": " << sqlite3_errmsg( db );
    Workspace::more_error() = out.str().c_str();
    DOMAIN_ERROR;
}

SqliteConnection::~SqliteConnection()
{
    if( sqlite3_close( db ) != SQLITE_OK ) {
        raise_sqlite_error( "Error closing database" );
    }
}

ArgListBuilder *SqliteConnection::make_prepared_query( const string &sql )
{
    SqliteArgListBuilder *builder = new SqliteArgListBuilder( this, sql );
    return builder;
}

ArgListBuilder *SqliteConnection::make_prepared_update( const string &sql )
{
    SqliteArgListBuilder *builder = new SqliteArgListBuilder( this, sql );
    return builder;
}

void SqliteConnection::run_simple( const string &sql )
{
    SqliteArgListBuilder builder( this, sql );
    builder.run_query( false );
}

void SqliteConnection::transaction_begin()
{
    run_simple( "begin" );
}

void SqliteConnection::transaction_commit()
{
    run_simple( "commit" );
}

void SqliteConnection::transaction_rollback()
{
    run_simple( "rollback" );
}
