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

#include "PostgresArgListBuilder.hh"

#include <string.h>
#include "ResultValue.hh"

PostgresArgListBuilder::PostgresArgListBuilder( PostgresConnection *connection_in, const string &sql_in )
    : connection( connection_in ), sql( sql_in )
{
}

void PostgresNullArg::update( Oid *types, const char **values, int *lengths, int *formats, int pos )
{
    
}

void PostgresArgListBuilder::append_string( const string &arg, int pos )
{
    Assert( static_cast<size_t>( pos ) == args.size() );
    args.push_back( new PostgresBindArg<const string>( arg ) );
}

void PostgresArgListBuilder::append_long( long arg, int pos )
{
    Assert( static_cast<size_t>( pos ) == args.size() );
    args.push_back( new PostgresBindArg<long>( arg ) );
}

void PostgresArgListBuilder::append_double( double arg, int pos )
{
    Assert( static_cast<size_t>( pos ) == args.size() );
    args.push_back( new PostgresBindArg<double>( arg ) );
}

void PostgresArgListBuilder::append_null( int pos )
{
    Assert( static_cast<size_t>( pos ) == args.size() );
    args.push_back( new PostgresNullArg() );
}

Token PostgresArgListBuilder::run_query()
{
    int n = args.size();
    DynArray( Oid, types, n );
    DynArray( const char *, values, n );
    DynArray( int, lengths, n );
    DynArray( int, formats, n );

    for( int i = 0 ; i < n ; i++ ) {
        PostgresArg *arg = args[i];
        arg->update( types, values, lengths, formats, i );
    }

    PGresult result = PQexecParams( connection->get_db(), sql.c_str(), n,
                                    types, values, lengths, formats,
                                    xx );
}
