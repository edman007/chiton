#include <main.hpp>
#include "util.hpp"
#include "chiton_config.hpp"
#include "mariadb.hpp"
#include "camera.hpp"
#include <unistd.h>
#include <thread>
#include <stdlib.h>

static char timezone_env[256];//if timezone is changed from default, we need to store it in memory for putenv()

void load_sys_cfg(Config &cfg) {
    std::string timezone = cfg.get_value("timezone");
    if (timezone.compare("")){
        LINFO("Setting Timezone to " + timezone);
        timezone = "TZ=" + timezone;
        auto len = timezone.copy(timezone_env, 0, sizeof(timezone_env) -1 );
        timezone_env[len] = '\0';
        putenv(timezone_env);
    }
    tzset();

}

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
    DatabaseResult *res = db.query("SELECT name, value FROM config WHERE camera IS NULL");
    while (res && res->next_row()){
        cfg.set_value(res->get_field(0), res->get_field(1));
    }
    delete res;


    //load system config
    load_sys_cfg(cfg);
    
    //Launch all cameras
    res = db.query("SELECT camera FROM config WHERE camera IS NOT NULL AND name = 'active' AND value = '1' GROUP BY camera");
    std::vector<Camera> cams;
    std::vector<std::thread> threads;
    while (res && res->next_row()){
        cams.emplace_back(res->get_field_long(0), db);//create camera
        threads.emplace_back(&Camera::run, cams.back());//start it

    }
    delete res;

    //camera maintance
    bool exit_requested = true;
    do {
        //we should check if they are running and restart anything that froze
        for (auto &c : cams){
            if (c.ping()){
                Util::log_msg(LOG_ERROR, "Thread Stalled");
            }
        }
        
    } while (!exit_requested);

    //shutdown all cams
    for (auto &c : cams){
        c.stop();
    }
    
    for (auto &t : threads){
        t.join();
    }

    
    return 0;
}





