#include "mariadb_result.hpp"
#include "util.hpp"


const std::string MariaDBResult::null_txt = "NULL";

MariaDBResult::~MariaDBResult(){
    mysql_free_result(res);
}

long MariaDBResult::num_rows(){
    return mysql_num_rows(res);
}

const std::string& MariaDBResult::col_name(unsigned int col){
    if (col_names.empty()){
        fetch_col_names();
    }

    try {
        return col_names.at(col);
    } catch (std::out_of_range &e){
        Util::log_msg(LOG_WARN, "Requested Column name out of range");
        return null_txt;
    }
}

int MariaDBResult::field_count(void){
    if (!col_count){
        col_count = mysql_num_fields(res);
    }
    return col_count;
}

void MariaDBResult::fetch_col_names(void){
    MYSQL_FIELD *field;
    while ((field = mysql_fetch_field(res))){
        col_names.emplace_back(field->name);
    }
}

bool MariaDBResult::next_row(void){
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
        Util::log_msg(LOG_WARN, "Requested column  out of range");
        return null_txt;
    }
}

bool MariaDBResult::field_is_null(unsigned int col){
    if (col < col_count && col_count != 0){
        return row[col] == NULL;
    }
    Util::log_msg(LOG_WARN, "Requesting NULL status of field that doesn't exist");
    return true;
}
