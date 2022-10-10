#ifndef __DATABASE_RESULT_HPP__
#define __DATABASE_RESULT_HPP__
/**************************************************************************
 *
 *     This file is part of Chiton.
 *
 *   Chiton is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Chiton is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Chiton.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   Copyright 2020-2022 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */
#include <map>
#include <string>

class DatabaseResult {
public:
    virtual int field_count(void) = 0;
    virtual long num_rows(void) = 0;
    virtual const std::string& col_name(unsigned int col) = 0;
    virtual bool next_row(void) = 0;//preps the next row for access, returns true if one exists
    virtual const std::string& get_field(unsigned int col) = 0;//get the data from the specified column as a string, data is valid until next call to next_row()
    virtual const long get_field_long(unsigned int col) = 0;//get the data from the specified column as a long
    virtual const long long get_field_ll(unsigned int col) = 0;//get the data from the specified column as a long long
    virtual const double get_field_double(unsigned int col) = 0;//get the data from the specified column as a double
    virtual bool field_is_null(unsigned int col) = 0;

    virtual ~DatabaseResult() {};
};

#endif
