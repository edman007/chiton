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
#include "mariadb_result.hpp"
#include "util.hpp"


const std::string MariaDBResult::null_txt = "NULL";

MariaDBResult::~MariaDBResult(){
    if (res){
        mysql_free_result(res);
    }
}

long MariaDBResult::num_rows(){
    if (!res){
        return 0;
    }
    
    return mysql_num_rows(res);
}

const std::string& MariaDBResult::col_name(unsigned int col){
    if (col_names.empty()){
        fetch_col_names();
    }

    try {
        return col_names.at(col);
    } catch (std::out_of_range &e){
        LWARN( "Requested Column name out of range");
        return null_txt;
    }
}

int MariaDBResult::field_count(void){
    if (!res){
        return 0;
    }
    if (!col_count){
        col_count = mysql_num_fields(res);
    }
    return col_count;
}

void MariaDBResult::fetch_col_names(void){
    MYSQL_FIELD *field;
    while (res && (field = mysql_fetch_field(res))){
        col_names.emplace_back(field->name);
    }
}

bool MariaDBResult::next_row(void){
    if (!res){
        return false;
    }
    row = mysql_fetch_row(res);
    //dump it all into col_data
    if (row){
        col_data.clear();
        for (int i = 0; i < field_count(); i++){
            if (row[i] == NULL){
                col_data.emplace_back("NULL");
            } else {
                col_data.emplace_back(row[i]);
            }
        }
    }
    return row != NULL;
}

const std::string& MariaDBResult::get_field(unsigned int col){
    try {
        return col_data.at(col);
    } catch (std::out_of_range &e){
        LWARN( "Requested column  out of range");
        return null_txt;
    }
}

const long MariaDBResult::get_field_long(unsigned int col){
    const std::string& val = get_field(col);
    if (!val.compare("")){
        //empty
        return 0;
    }

    try {
        return std::stol(val);
    } catch (const std::invalid_argument& ia){
        LWARN( "database  value " + std::to_string(col) + " ( " + val + " ) must be an integer");
    } catch (const std::out_of_range& ia) {
        LWARN( "database value " + std::to_string(col) + " ( " + val + " ) is out of range ");
    }
    
    return 0;

}

const double MariaDBResult::get_field_double(unsigned int col){
    const std::string& val = get_field(col);
    if (!val.compare("")){
        //empty
        return 0;
    }

    try {
        return std::stod(val);
    } catch (const std::invalid_argument& ia){
        LWARN( "database value " + std::to_string(col) + " ( " + val + " ) must be a double");
    } catch (const std::out_of_range& ia) {
        LWARN( "database value " + std::to_string(col) + " ( " + val + " ) is out of range ");
    }
    
    return 0;

}

bool MariaDBResult::field_is_null(unsigned int col){
    if (col < col_count && col_count != 0){
        return row[col] == NULL;
    }
    LWARN( "Requesting NULL status of field that doesn't exist");
    return true;
}
