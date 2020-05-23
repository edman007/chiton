#ifndef __MARIADB_HPP__
#define __MARIADB_HPP__
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

#include "database.hpp"
#include <mysql/mysql.h>
#include "mariadb_result.hpp"
#include <mutex>

class MariaDB : public Database {
public:
    int connect(const std::string& server, const std::string& db, const std::string& user, const std::string& pass, const int port, const std::string& socket);
    MariaDB();
    ~MariaDB();

    //escape a string
    std::string escape(const std::string& str);
    MariaDBResult* query(const std::string& query) __wur;
    MariaDBResult* query(const std::string& query, long* affected_rows, long *insert_id);
    
private:
    MYSQL *conn;//our server connection
    MariaDBResult* query_nolock(const std::string& sql);
    std::mutex mtx;
};
#endif
