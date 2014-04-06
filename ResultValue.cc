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

#include "ResultValue.hh"

#include "Value.hh"
#include "IntCell.hh"
#include "FloatCell.hh"
#include "CharCell.hh"
#include "PointerCell.hh"

static const UCS_string ucs_string_from_string( const std::string &string )
{
    size_t length = string.size();
    const char *buf = string.c_str();
    UTF8_string utf( (const UTF8 *)buf, length );
    return UCS_string( utf );
}

static Value_P make_string_cell( const std::string &string, const char *loc )
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

void IntResultValue::update( Cell *cell )
{
    new (cell) IntCell( value );
}

void DoubleResultValue::update( Cell *cell )
{
    new (cell) FloatCell( value );
}

void StringResultValue::update( Cell *cell )
{
    new (cell) PointerCell( make_string_cell( value, LOC ) );
}

void ResultRow::add_values( sqlite3_stmt *statement )
{
    int n = sqlite3_column_count( statement );
    for( int i = 0 ; i < n ; i++ ) {
        ResultValue *value;
        switch( sqlite3_column_type( statement, i ) ) {
        case SQLITE_INTEGER:
            value = new IntResultValue( sqlite3_column_int( statement, i ) );
            break;
        case SQLITE_FLOAT:
            value = new DoubleResultValue( sqlite3_column_double( statement, i ) );
            break;
        case SQLITE_TEXT:
            value = new StringResultValue( reinterpret_cast<const char *>( sqlite3_column_text( statement, i ) ) );
            break;
        case SQLITE_BLOB:
            value = new NullResultValue();
            break;
        case SQLITE_NULL:
            value = new NullResultValue();
            break;
        }
    }
}
