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
#include "SqliteResultValue.hh"
#include "SqliteArgListBuilder.hh"

void SqliteConnection::raise_sqlite_error( const string &message )
{
    stringstream out;
    out << message << ": " << sqlite3_errmsg( db );
    Workspace::more_error() = out.str().c_str();
    DOMAIN_ERROR;
}

SqliteConnection::SqliteConnection( sqlite3 *db_in )
    : db( db_in )
{
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

void SqliteConnection::fill_tables( vector<string> &tables )
{
    sqlite3_stmt *statement;
    if( sqlite3_prepare_v2( get_db(), "select name from sqlite_master where type = 'table'", -1,
                            &statement, NULL ) != SQLITE_OK ) {
        raise_sqlite_error( "Error getting table names" );
    }

    SqliteStmtWrapper statement_wrapper( statement );
    int result;
    while( (result = sqlite3_step( statement_wrapper.get_statement() )) != SQLITE_DONE ) {
        if( result != SQLITE_ROW ) {
            raise_sqlite_error( "Error getting next table name" );
        }

        int type = sqlite3_column_type( statement, 0 );
        if( type != SQLITE_TEXT ) {
            raise_sqlite_error( "Table name is not a text column" );
        }

        tables.push_back( reinterpret_cast<const char *>( sqlite3_column_text( statement, 0 ) ) );
    }
}

void SqliteConnection::fill_cols( const string &table, vector<ColumnDescriptor> &cols )
{
    sqlite3_stmt *statement;
    char *statement_content = sqlite3_mprintf( "pragma table_info('%q')", table.c_str() );
    if( sqlite3_prepare_v2( get_db(), statement_content, -1, &statement, NULL ) != SQLITE_OK ) {
        raise_sqlite_error( "Error getting table names" );
    }

    SqliteStmtWrapper statement_wrapper( statement );
    int result;
    while( (result = sqlite3_step( statement_wrapper.get_statement() )) != SQLITE_DONE ) {
        cols.push_back( ColumnDescriptor( reinterpret_cast<const char *>( sqlite3_column_text( statement, 1 ) ),
                                          reinterpret_cast<const char *>( sqlite3_column_text( statement, 2 ) ) ) );
    }
}

const string SqliteConnection::make_positional_param( int )
{
    return "?";
}
