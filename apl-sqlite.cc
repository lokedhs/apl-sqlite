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

#include <vector>
#include <map>
#include <typeinfo>

#include <string.h>

#include "Connection.hh"
#include "ResultValue.hh"
#include "Provider.hh"
#include "SqliteProvider.hh"
#include "PostgresProvider.hh"

typedef vector<Connection *> DbConnectionVector;

map<const string, Provider *> providers;
DbConnectionVector connections;

extern "C" {
    void *get_function_mux( const char *function_name );
}

static void add_provider( Provider *provider )
{
    providers.insert( pair<const string, Provider *>( provider->get_name(), provider ) );
}

static void init_provider_map( void )
{
    add_provider( new SqliteProvider() );
    add_provider( new PostgresProvider() );
}

static Token list_functions( ostream &out )
{
    out << "Available function numbers:" << endl
        << "name FN[1] args     - open database. Returns reference ID" << endl
        << "FN[2] ref           - close database" << endl
        << "query FN[3,db] params  - send SQL query (remaining params are bind args)" << endl
        << "query FN[4,db] params  - send SQL update (remaining params are bind args)" << endl
        << "FN[5] ref           - begin transaction" << endl
        << "FN[6] ref           - commit transaction" << endl
        << "FN[7] ref           - rollback transaction" << endl;
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

static Token open_database( Value_P A, Value_P B )
{
    if( !A->is_apl_char_vector() ) {
        Workspace::more_error() = "Illegal database name";
        VALUE_ERROR;
    }
    string type = to_string( A->get_UCS_ravel() );
    map<const string, Provider *>::iterator provider_iterator = providers.find( type );
    if( provider_iterator == providers.end() ) {
        stringstream out;
        out << "Unknown database type: " << type;
        Workspace::more_error() = out.str().c_str();
        VALUE_ERROR;
    }

    int connection_index = find_free_connection();
    connections[connection_index] = provider_iterator->second->open_database( B );

    return Token( TOK_APL_VALUE1, Value_P( new Value( IntCell( connection_index ), LOC ) ) );
}

static void throw_illegal_db_id( void )
{
    Workspace::more_error() = "Illegal database id";
    DOMAIN_ERROR;
}

static Connection *db_id_to_connection( int db_id )
{
   if( db_id < 0 || db_id >= (int)connections.size() ) {
        throw_illegal_db_id();
    }
    Connection *conn = connections[db_id];
    if( conn == NULL ) {
        throw_illegal_db_id();
    }

    return conn;
}

static Connection *value_to_db_id( APL_Float qct, Value_P value )
{
    if( !value->is_int_skalar( qct ) ) {
        throw_illegal_db_id();
    }

    int db_id = value->get_ravel( 0 ).get_int_value();
    return db_id_to_connection( db_id );
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

static Token run_generic( Connection *conn, Value_P A, Value_P B, bool query )
{
    if( !A->is_char_string() ) {
        Workspace::more_error() = "Illegal query argument type";
        VALUE_ERROR;
    }

    string statement = to_string( A->get_UCS_ravel() );
    ArgListBuilder *builder;
    if( query ) {
        builder = conn->make_prepared_query( statement );
    }
    else {
        builder = conn->make_prepared_update( statement );
    }
    auto_ptr<ArgListBuilder> arg_list( builder );

    const Shape &shape = B->get_shape();
    if( shape.get_rank() == 0 || shape.get_rank() == 1 ) {
        int num_args = shape.get_volume();
        for( int i = 0 ; i < num_args ; i++ ) {
            const Cell &cell = B->get_ravel( i );
            if( cell.is_integer_cell() ) {
                arg_list->append_long( cell.get_int_value(), i );
            }
            else if( cell.is_float_cell() ) {
                arg_list->append_double( cell.get_real_value(), i );
            }
            else {
                Value_P value = cell.to_value( LOC );
                if( value->get_shape().get_volume() == 0 ) {
                    arg_list->append_null( i );
                }
                else if( value->is_char_string() ) {
                    arg_list->append_string( to_string( value->get_UCS_ravel() ), i );
                }
                else {
                    stringstream out;
                    out << "Illegal data type in argument " << i << " of arglist";
                    Workspace::more_error() = out.str().c_str();
                    VALUE_ERROR;
                }
            }
        }

        return arg_list->run_query();
    }
    else {
        Workspace::more_error() = "Bind params have illegal rank";
        VALUE_ERROR;
    }
}

static Token run_query( Connection *conn, Value_P A, Value_P B )
{
    return run_generic( conn, A, B, true );
}

static Token run_update( Connection *conn, Value_P A, Value_P B )
{
    return run_generic( conn, A, B, false );
}

static Token run_transaction_begin( APL_Float qct, Value_P B )
{
    Connection *conn = value_to_db_id( qct, B );
    conn->transaction_begin();
    return Token( TOK_APL_VALUE1, Value::Idx0_P );
}

static Token run_transaction_commit( APL_Float qct, Value_P B )
{
    Connection *conn = value_to_db_id( qct, B );
    conn->transaction_commit();
    return Token( TOK_APL_VALUE1, Value::Idx0_P );
}

static Token run_transaction_rollback( APL_Float qct, Value_P B )
{
    Connection *conn = value_to_db_id( qct, B );
    conn->transaction_rollback();
    return Token( TOK_APL_VALUE1, Value::Idx0_P );
}

Fun_signature get_signature()
{
    init_provider_map();
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

    case 2:
        return close_database( qct, B );

    case 5:
        return run_transaction_begin( qct, B );

    case 6:
        return run_transaction_commit( qct, B );

    case 7:
        return run_transaction_rollback( qct, B );

    default:
        Workspace::more_error() = "Illegal function number";
        DOMAIN_ERROR;
    }
}

static Connection *param_to_db( APL_Float qct, Value_P X )
{
    const Shape &shape = X->get_shape();
    if( shape.get_volume() != 2 ) {
        Workspace::more_error() = "Database id missing from axis parameter";
        RANK_ERROR;
    }
    return db_id_to_connection( X->get_ravel( 1 ).get_near_int( qct ) );
}

Token eval_AXB(const Value_P A, const Value_P X, const Value_P B)
{
    const APL_Float qct = Workspace::get_CT();
    const int function_number = X->get_ravel( 0 ).get_near_int( qct );

    switch( function_number ) {
    case 0:
        return list_functions( CERR );

    case 1:
        return open_database( A, B );

    case 3:
        return run_query( param_to_db( qct, X ), A, B );

    case 4:
        return run_update( param_to_db( qct, X ), A, B );

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

static const UCS_string ucs_string_from_string( const std::string &string )
{
    size_t length = string.size();
    const char *buf = string.c_str();
    UTF8_string utf( (const UTF8 *)buf, length );
    return UCS_string( utf );
}

Value_P make_string_cell( const std::string &string, const char *loc )
{
    UCS_string s = ucs_string_from_string( string );
    Shape shape( s.size() );
    Value_P cell( new Value( shape, loc ) );
    for( int i = 0 ; i < s.size() ; i++ ) {
        new (cell->next_ravel()) CharCell( s[i] );
    }
    cell->check_value( loc );
    return cell;
}
