#ifndef __DATABASE_HPP__
#define __DATABASE_HPP__
#include <string>

#include "database_result.hpp"

class Database {
public:
    /*
     * Connect to the database, return 0 on success, non-zero on error
     */
    virtual int connect(const std::string& server, const std::string& db, const std::string& user, const std::string& pass, const int port, const std::string& socket) = 0;

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
