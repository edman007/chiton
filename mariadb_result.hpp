#ifndef __MARIADB_RESULT_HPP__
#define __MARIADB_RESULT_HPP__
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
 *   Copyright 2020 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "database_result.hpp"
#include <mysql/mysql.h>
#include <vector>

class MariaDBResult : public DatabaseResult {
public:

    MariaDBResult(MYSQL_RES *res) : res(res) {};
    ~MariaDBResult();

    int field_count(void);
    long num_rows(void);
    const std::string& col_name(unsigned int col);
    const std::string& get_field(unsigned int col);
    const long get_field_long(unsigned int col);//get the data from the specified column as a long
    const double get_field_double(unsigned int col);//get the data from the specified column as a double
    
    bool next_row(void);
    bool field_is_null(unsigned int col);
private:
    MYSQL_RES *res;
    MYSQL_ROW row = NULL;
    
    unsigned int col_count = 0;
    std::vector<std::string> col_names;
    std::vector<std::string> col_data;
    void fetch_col_names(void);//fills the vector col_names with the names of all columns

    static const std::string null_txt;//we return this NULL string when we hit a bad value to keep on going
};

#endif
