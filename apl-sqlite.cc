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

#include <vector>

#include <string.h>
#include <sqlite3.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wmismatched-tags"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include "Native_interface.hh"
#pragma GCC diagnostic pop

#include "Connection.hh"

typedef vector<Connection> DbConnectionVector;

DbConnectionVector connections;

extern "C" {
    void *get_function_mux( const char *function_name );
}

static Token list_functions( ostream &out )
{
    out << "Available function numbers:" << endl
        << "FN[1] name          - open database. Returns reference ID" << endl
        << "FN[2] ref           - close database" << endl
        << "ref FN[3] query ... - sent SQL query (remaining params are bind args)";
    return Token(TOK_APL_VALUE1, Value::Str0_P);
}

static int find_free_connection( void )
{
    for( int i = 0 ; i < static_cast<int>( connections.size() ) ; i++ ) {
        if( !connections[i].is_active() ) {
            return i;
        }
    }

    Connection conn;
    connections.push_back( conn );
    return connections.size() - 1;
}

static Token open_database( Value_P B )
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

    int connection_index = find_free_connection();
    Connection &conn = connections[connection_index];
    conn.set_db( db );

    return Token( TOK_APL_VALUE1, Value_P( new Value( IntCell( connection_index ), LOC ) ) );
}

static void throw_illegal_db_id( void )
{
    Workspace::more_error() = "Illegal database id";
    DOMAIN_ERROR;
}

static Connection &value_to_db_id( APL_Float qct, Value_P value )
{
    if( !value->is_int_skalar( qct ) ) {
        throw_illegal_db_id();
    }

    int db_id = value->get_ravel( 0 ).get_int_value();
    if( db_id < 0 || db_id >= (int)connections.size() ) {
        throw_illegal_db_id();
    }
    Connection &conn = connections[db_id];
    if( !conn.is_active() ) {
        throw_illegal_db_id();
    }

    return conn;
}

static void raise_sqlite_error( sqlite3 *db, const string &message )
{
    stringstream out;
    out << message << ": " << sqlite3_errmsg( db );
    Workspace::more_error() = out.str().c_str();
    DOMAIN_ERROR;
}

static Token close_database( APL_Float qct, Value_P B )
{
    if( !B->is_int_skalar( qct ) ) {
        Workspace::more_error() = "Close database command requires database id as argument";
        DOMAIN_ERROR;
    }

    int db_id = B->get_ravel( 0 ).get_int_value();
    if( db_id < 0 || db_id >= (int)connections.size() ) {
        throw_illegal_db_id();
    }
    Connection &conn = connections[db_id];
    if( !conn.is_active() ) {
        throw_illegal_db_id();
    }

    sqlite3 *db = conn.get_db();
    if( sqlite3_close( db ) != SQLITE_OK ) {
        raise_sqlite_error( db, "Error closing database" );
    }

    conn.set_db( NULL );

    return Token( TOK_APL_VALUE1, Value::Str0_P );
}

static string make_query( APL_Float qct, Value_P value )
{
    if( value->is_char_string() ) {
        return value->get_UCS_ravel().to_string();
    }
    else {
        const Shape &shape = value->get_shape();
        if( shape.get_rank() == 1 ) {
            Workspace::more_error() = "Rank 1 argument should be a string";
            DOMAIN_ERROR;
        }
        else if( shape.get_rank() == 2 ) {
            Value_P statement_value = value->get_ravel( 0 ).to_value( LOC );
            if( !statement_value->is_char_string() ) {
                Workspace::more_error() = "SQL statement does not have the right type";
                DOMAIN_ERROR;
            }

            string statement = statement_value->get_UCS_ravel().to_string();
            vector<string> args;
            Assert_fatal( shape.get_volume() >= 1 );
            int num_args = shape.get_volume() - 1;
            for( int i = 0 ; i < num_args ; i++ ) {
                Value_P arg_value = value->get_ravel( i + 1 ).to_value( LOC );
                if( !arg_value->is_char_string() ) {
                    Workspace::more_error() = "Argument should be a string";
                    DOMAIN_ERROR;
                }
                args.push_back( arg_value->get_UCS_ravel().to_string() );
            }

            if( !args.empty() ) {
                Workspace::more_error() = "Bind args are not yet implemented";
                DOMAIN_ERROR;
            }

            return statement;
        }
        else {
            Workspace::more_error() = "Illegal query argument";
            DOMAIN_ERROR;
        }
    }
}

Token run_query( APL_Float qct, Value_P A, Value_P B )
{
    Connection &conn = value_to_db_id( qct, A );
    string sql = make_query( qct, B );
    
    sqlite3_stmt *statement;
    const char *sql_charptr = sql.c_str();
    if( sqlite3_prepare_v2( conn.get_db(),
                            sql_charptr, strlen( sql_charptr ) + 1,
                            &statement, NULL ) != SQLITE_OK ) {
        raise_sqlite_error( conn.get_db(), "Error preparing query" );
    }

    Shape result_shape( 1, 1 );
    Value_P db_result_value( new Value( result_shape, LOC ) );

    int result;
    while( (result = sqlite3_step( statement )) != SQLITE_DONE ) {
        if( result != SQLITE_ROW ) {
            raise_sqlite_error( conn.get_db(), "Error reading sql result" );
        }
        CERR << "row" << endl;
    }

    CERR << "end of results" << endl;

    new (db_result_value->next_ravel()) IntCell( 1000 );

    sqlite3_finalize( statement );

    db_result_value->check_value( LOC );
    return Token( TOK_APL_VALUE1, db_result_value );
}

Fun_signature get_signature()
{
    return SIG_Z_A_F2_B;
}

void close_fun( Cause cause )
{
}

Token eval_AB(Value_P A, Value_P B)
{
    return list_functions( COUT );
}

Token eval_XB(Value_P X, Value_P B)
{
    const APL_Float qct = Workspace::get_CT();
    const int function_number = X->get_ravel( 0 ).get_near_int( qct );

    switch( function_number ) {
    case 0:
        return list_functions( CERR );

    case 1:
        return open_database( B );

    case 2:
        return close_database( qct, B );

    default:
        Workspace::more_error() = "Illegal function number";
        DOMAIN_ERROR;
    }
}

Token eval_AXB(const Value_P A, const Value_P X, const Value_P B)
{
    const APL_Float qct = Workspace::get_CT();
    const int function_number = X->get_ravel( 0 ).get_near_int( qct );

    switch( function_number ) {
    case 0:
        return list_functions( CERR );

    case 3:
        return run_query( qct, A, B );

    default:
        Workspace::more_error() = "Illegal function number";
        DOMAIN_ERROR;
    }
}

void *get_function_mux( const char *function_name )
{
    if( strcmp( function_name, "get_signature" ) == 0 ) return (void *)&get_signature;
//    if( strcmp( function_name, "eval_B" ) == 0 )        return (void *)&eval_B;
    if( strcmp( function_name, "eval_AB" ) == 0 )       return (void *)&eval_AB;
    if( strcmp( function_name, "eval_XB" ) == 0 )       return (void *)&eval_XB;
    if( strcmp( function_name, "eval_AXB" ) == 0 )      return (void *)&eval_AXB;
    if( strcmp( function_name, "close_fun" ) == 0 )     return (void *)&close_fun;
    return 0;
}
