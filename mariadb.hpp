#ifndef __MARIADB_HPP__
#define __MARIADB_HPP__

#include "database.hpp"
#include <mysql/mysql.h>
#include "mariadb_result.hpp"


class MariaDB : public Database {
public:
    int connect(const std::string& server, const std::string& db, const std::string& user, const std::string& pass, const int port, const std::string& socket);
    MariaDB();
    ~MariaDB();

    //escape a string
    std::string escape(const std::string& str);
    MariaDBResult* query(const std::string& query) __wur;

    long affected_rows(void);
private:
    MYSQL *conn;//our server connection
    
};
#endif
