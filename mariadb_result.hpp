#ifndef __MARIADB_RESULT_HPP__
#define __MARIADB_RESULT_HPP__

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
