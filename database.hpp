#ifndef __DATABASE_HPP__
#define __DATABASE_HPP__
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
#include <string>

#include "database_result.hpp"

class Database {
public:
    /*
     * Connect to the database, return 0 on success, non-zero on error
     */
    virtual int connect(const std::string& server, const std::string& db, const std::string& user, const std::string& pass, const int port, const std::string& socket) = 0;

    /*
     * Disconnect from the database, following calls to connect() will succeed
     */
    virtual void close(void) = 0;

    //escape a string
    virtual std::string escape(const std::string& str) = 0;

    //returns a result if there is one, caller must call delete on it when done
    virtual DatabaseResult* query(const std::string& query) __wur = 0;

    //returns a result if there is one, caller must call delete on it when done
    //if affected_rows is not null, will be set to the affected_row count in a thread safe manner
    //if insert_id is not null, will be set to the last insert_id in a thread safe manner
    virtual DatabaseResult* query(const std::string& query, long* affected_rows, long *insert_id) __wur = 0;

    virtual ~Database() {};
    
};

#endif
