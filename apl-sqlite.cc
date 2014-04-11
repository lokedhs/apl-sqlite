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

#include <vector>

#include <string.h>

#include "Connection.hh"
#include "ResultValue.hh"

typedef vector<Connection *> DbConnectionVector;

DbConnectionVector connections;

extern "C" {
    void *get_function_mux( const char *function_name );
}

static Token list_functions( ostream &out )
{
    out << "Available function numbers:" << endl
        << "FN[1] name          - open database. Returns reference ID" << endl
        << "FN[2] ref           - close database" << endl
        << "ref FN[3] query ... - send SQL query (remaining params are bind args)" << endl;
    return Token(TOK_APL_VALUE1, Value::Str0_P);
}

static int find_free_connection( void )
{
    for( int i = 0 ; i < static_cast<int>( connections.size() ) ; i++ ) {
        if( connections[i] == NULL ) {
            return i;
        }
    }

    connections.push_back( NULL );
    return connections.size() - 1;
}

static Token open_database( Value_P B )
{
    Connection *connection = create_sqlite_connection( B );

    int connection_index = find_free_connection();
    connections[connection_index] = connection;

    return Token( TOK_APL_VALUE1, Value_P( new Value( IntCell( connection_index ), LOC ) ) );
}

static void throw_illegal_db_id( void )
{
    Workspace::more_error() = "Illegal database id";
    DOMAIN_ERROR;
}

static Connection *value_to_db_id( APL_Float qct, Value_P value )
{
    if( !value->is_int_skalar( qct ) ) {
        throw_illegal_db_id();
    }

    int db_id = value->get_ravel( 0 ).get_int_value();
    if( db_id < 0 || db_id >= (int)connections.size() ) {
        throw_illegal_db_id();
    }
    Connection *conn = connections[db_id];
    if( conn == NULL ) {
        throw_illegal_db_id();
    }

    return conn;
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
    Connection *conn = connections[db_id];
    if( conn == NULL ) {
        throw_illegal_db_id();
    }

    connections[db_id] = NULL;
    delete conn;

    return Token( TOK_APL_VALUE1, Value::Str0_P );
}

Token run_query( APL_Float qct, Value_P A, Value_P B )
{
    Connection *conn = value_to_db_id( qct, A );

    if( B->is_char_string() ) {
        auto_ptr<ArgListBuilder> arg_list( conn->make_arg_list_builder() );
        return conn->run_query( B->get_UCS_ravel().to_string(), arg_list.get() );
    }
    else {
        const Shape &shape = B->get_shape();
        if( shape.get_rank() == 0 ) {
            Workspace::more_error() = "Query arguments can't be a scalar";
            RANK_ERROR;
        }
        else if( shape.get_rank() == 1 ) {
            Value_P statement_value = B->get_ravel( 0 ).to_value( LOC );
            if( !statement_value->is_char_string() ) {
                Workspace::more_error() = "SQL statement does not have the right type";
                DOMAIN_ERROR;
            }

            string statement = statement_value->get_UCS_ravel().to_string();
            Assert_fatal( shape.get_volume() >= 1 );

            auto_ptr<ArgListBuilder> arg_list( conn->make_arg_list_builder() );

            int num_args = shape.get_volume() - 1;
            for( int i = 0 ; i < num_args ; i++ ) {
                const Cell &cell = B->get_ravel( i + 1 );
                if( cell.is_integer_cell() ) {
                    arg_list->append_long( cell.get_int_value() );
                }
                else if( cell.is_float_cell() ) {
                    arg_list->append_long( cell.get_real_value() );
                }
                else {
                    Value_P value = cell.to_value( LOC );
                    if( value->get_shape().get_volume() == 0 ) {
                        arg_list->append_null();
                    }
                    if( value->is_char_string() ) {
                        arg_list->append_string( value->get_UCS_ravel().to_string() );
                    }
                    else {
                        stringstream out;
                        out << "Illegal data type in argument " << i << " of arglist";
                        Workspace::more_error() = out.str().c_str();
                        DOMAIN_ERROR;
                    }
                }
            }

            return conn->run_query( statement, arg_list.get() );
        }
        else {
            Workspace::more_error() = "Illegal query argument";
            DOMAIN_ERROR;
        }
    }
}

Fun_signature get_signature()
{
    return SIG_Z_A_F2_B;
}

void close_fun( Cause cause )
{
}

Token eval_B( Value_P B )
{
    return list_functions( COUT );
}

Token eval_AB( Value_P A, Value_P B )
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
    if( strcmp( function_name, "eval_B" ) == 0 )        return (void *)&eval_B;
    if( strcmp( function_name, "eval_AB" ) == 0 )       return (void *)&eval_AB;
    if( strcmp( function_name, "eval_XB" ) == 0 )       return (void *)&eval_XB;
    if( strcmp( function_name, "eval_AXB" ) == 0 )      return (void *)&eval_AXB;
    if( strcmp( function_name, "close_fun" ) == 0 )     return (void *)&close_fun;
    return 0;
}
