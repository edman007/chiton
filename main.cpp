#include <main.hpp>
#include "util.hpp"
#include "chiton_config.hpp"
#include "mariadb.hpp"

int main (int argc, char **argv){
    Util::log_msg(LOG_INFO, "Starting Chiton...");
    Util::log_msg(LOG_INFO, std::string("\tVersion ") + GIT_VER);
    Util::log_msg(LOG_INFO, std::string("\tBuilt ") + BUILD_DATE);
    
    Config cfg;
    cfg.load_default_config();
    MariaDB db;
    db.connect(cfg.get_value("db-host"), cfg.get_value("db-database"), cfg.get_value("db-user"), cfg.get_value("db-password"),
               cfg.get_value_int("db-port"), cfg.get_value("db-socket"));

    //load the default config from the database
    DatabaseResult *res = db.query("SELECT name, value FROM config WHERE camera = NULL");
    while (res && res->next_row()){
        cfg.set_value(res->get_field(0), res->get_field(1));
    }
    delete res;
    
    
    return 0;
}





