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
#include "Sqlite3Connection.hh"
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

SqliteConnection *create_sqlite_connection( Value_P B )
{
    if( !B->is_char_string() ) {
        Workspace::more_error() = "SQLite database connect argument must be a single string";
        DOMAIN_ERROR;
    }

    string filename = B->get_UCS_ravel().to_string();
    sqlite3 *db;
    if( sqlite3_open( filename.c_str(), &db ) != SQLITE_OK ) {
        stringstream out;
        out << "Error opening database: " << sqlite3_errmsg( db );
        Workspace::more_error() = out.str().c_str();
        DOMAIN_ERROR;
    }

    return new SqliteConnection( db );
}

Token SqliteConnection::run_query( const string &sql, ArgListBuilder *arg_list )
{
    sqlite3_stmt *statement;
    const char *sql_charptr = sql.c_str();
    if( sqlite3_prepare_v2( db, sql_charptr, strlen( sql_charptr ) + 1, &statement, NULL ) != SQLITE_OK ) {
        raise_sqlite_error( "Error preparing query" );
    }

    SqliteArgListBuilder *sqlite_arg_list = static_cast<SqliteArgListBuilder *>( arg_list );
    sqlite_arg_list->bind_args( statement );

    vector<ResultRow> results;
    int result;
    while( (result = sqlite3_step( statement )) != SQLITE_DONE ) {
        if( result != SQLITE_ROW ) {
            raise_sqlite_error( "Error reading sql result" );
        }

        ResultRow row;
        row.add_values( statement );
        results.push_back( row );
    }

    Value_P db_result_value;
    int row_count = results.size();
    if( row_count > 0 ) {
        int col_count = results[0].get_values().size();
        Shape result_shape( row_count, col_count );
        db_result_value = new Value( result_shape, LOC );
        for( vector<ResultRow>::iterator row_iterator = results.begin() ; row_iterator != results.end() ; row_iterator++ ) {
            const vector<const ResultValue *> &row = row_iterator->get_values();
            for( vector<const ResultValue *>::const_iterator col_iterator = row.begin() ; col_iterator != row.end() ; col_iterator++ ) {
                (*col_iterator)->update( db_result_value->next_ravel() );
            }
        }
    }
    else {
        db_result_value = Value::Idx0_P;
    }

    sqlite3_finalize( statement );

    db_result_value->check_value( LOC );
    return Token( TOK_APL_VALUE1, db_result_value );
}

Token SqliteConnection::run_update( const string &sql, ArgListBuilder *arg_list )
{
    return run_query( sql, arg_list );
}

ArgListBuilder *SqliteConnection::make_arg_list_builder( void )
{
    SqliteArgListBuilder *builder = new SqliteArgListBuilder();
    return builder;
}
