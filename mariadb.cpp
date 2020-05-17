#include "mariadb.hpp"
#include "util.hpp"

MariaDB::MariaDB(){
    conn = NULL;
    
    conn = mysql_init(conn);
}

MariaDB::~MariaDB(){
    mysql_close(conn);
    conn = NULL;
}

int MariaDB::connect(const std::string& server, const std::string& db, const std::string& user, const std::string& pass, const int port, const std::string& socket){
    int flags = 0;
    const char *sock;

    //doesn't work with an empty string
    if (socket.compare("")){
        sock = socket.c_str();
    } else {
        sock = 0;
    }
    if (!mysql_real_connect(conn, server.c_str(), user.c_str(), pass.c_str(), db.c_str(), port, sock, flags)){
        Util::log_msg(LOG_ERROR, "Failed to connect to database: Error: " + std::string(mysql_error(conn)));
        return (int)mysql_errno(conn);
    }
    Util::log_msg(LOG_INFO, "Connected to Database");
    return 0;
}


std::string MariaDB::escape(const std::string& str){
    char out_str[str.length()*2+1];
    mysql_real_escape_string(conn, out_str, str.c_str(), str.length());
    
    return std::string(out_str);//is this a good idea to copy the string...
    
}

MariaDBResult* MariaDB::query_nolock(const std::string& sql){
    int ret = mysql_real_query(conn, sql.c_str(), sql.length());
    if (ret){
        //error
        Util::log_msg(LOG_WARN, "Query Failed: " + std::string(mysql_error(conn)));
        return NULL;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    if (res){
        return new MariaDBResult(res);
    } else {
        return NULL;
    }
}


MariaDBResult* MariaDB::query(const std::string& sql){
    return query(sql, NULL, NULL);
}

MariaDBResult* MariaDB::query(const std::string& sql, long* affected_rows, long *insert_id){
    mtx.lock();
    MariaDBResult *ret = query_nolock(sql);
    if (affected_rows){
        *affected_rows = mysql_affected_rows(conn);
    }
    if (insert_id){
        *insert_id = mysql_insert_id(conn);
    }
    mtx.unlock();
    return ret;

}
