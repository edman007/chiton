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
#include "chiton_config.hpp"
#include "util.hpp"
#include <fstream>
#include "config_parser.hpp"

bool Config::load_default_config(void){
    return load_config("config/chiton.cfg");
}

bool Config::load_config(const std::string& path){
    std::ifstream ifs;
    ifs.open(path);
    if (ifs.fail()){
        Util::log_msg(LOG_WARN, "Failued to open config file `" + path + "`");
        return false;//didn't work
    }
    ConfigParser parser(*this, ifs);
    parser.parse();
    return true;
}

    
const std::string Config::get_value(const std::string& key){
    if (!key.compare("")){
        Util::log_msg(LOG_WARN, "Code is requesting a null key");
        return "";
    }
    
    return cfg_db[key];
}

void Config::set_value(const std::string& key, const std::string& value){
    if (key.compare("")){
        cfg_db[key] = value;
    } else {
        Util::log_msg(LOG_INFO, "Ignoring empty key");
    }
}


int Config::get_value_int(const std::string& key){
    const std::string& val = get_value(key);
    if (!val.compare("")){
        //empty
        return 0;
    }

    try {
        return std::stoi(val);
    } catch (const std::invalid_argument& ia){
        Util::log_msg(LOG_WARN, "Config value " + key + " ( " + val + " ) must be an integer");
    } catch (const std::out_of_range& ia) {
        Util::log_msg(LOG_WARN, "Config value " + key + " ( " + val + " ) is out of range ");
    }
    
    return 0;
}

double Config::get_value_double(const std::string& key){
    const std::string& val = get_value(key);
    if (!val.compare("")){
        //empty
        return 0;
    }

    try {
        return std::stod(val);
    } catch (const std::invalid_argument& ia){
        Util::log_msg(LOG_WARN, "Config value " + key + " ( " + val + " ) must be an integer");
    } catch (const std::out_of_range& ia) {
        Util::log_msg(LOG_WARN, "Config value " + key + " ( " + val + " ) is out of range ");
    }
    
    return 0;

}
