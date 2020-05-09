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

    //returns affected row count of last query
    virtual long affected_rows(void) = 0;

    virtual ~Database() {};
    
};

#endif
