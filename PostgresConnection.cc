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
#include "PostgresConnection.hh"
#include "PostgresArgListBuilder.hh"

PostgresConnection::~PostgresConnection()
{
    PQfinish( db );
}

ArgListBuilder *PostgresConnection::make_prepared_query( const string &sql )
{
    return new PostgresArgListBuilder( this, sql );
}

ArgListBuilder *PostgresConnection::make_prepared_update( const string &sql )
{
    return new PostgresArgListBuilder( this, sql );    
}
