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

MariaDBResult* MariaDB::query(const std::string& query){
    int ret = mysql_real_query(conn, query.c_str(), query.length());
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

long MariaDB::affected_rows(void){
    return mysql_affected_rows(conn);
}
