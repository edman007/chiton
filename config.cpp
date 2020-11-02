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
#include "setting.hpp"

Config::Config(){
    set_value("camera-id", std::string("-1"));//set the camera ID so other tools can use it
}

bool Config::load_config(const std::string& path){
    std::ifstream ifs;
    ifs.open(path);
    if (ifs.fail()){
        LWARN( "Failed to open config file `" + path + "`");
        return false;//didn't work
    }
    ConfigParser parser(*this, ifs);
    parser.parse();
    return true;
}

    
const std::string& Config::get_value(const std::string& key){
    if (!key.compare("")){
        LWARN( "Code is requesting a null key");
        return EMPTY_STR;
    }

    auto ret = cfg_db.find(key);
    if (ret == cfg_db.end()){
        return get_default_value(key);
    } else {
        return ret->second;
    }
}

void Config::set_value(const std::string& key, const std::string& value){
    if (key.compare("")){
        cfg_db[key] = value;
    } else {
        LINFO( "Ignoring empty key");
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
        LWARN( "Config value " + key + " ( " + val + " ) must be an integer");
    } catch (const std::out_of_range& ia) {
        LWARN( "Config value " + key + " ( " + val + " ) is out of range ");
    }
    
    return 0;
}

long Config::get_value_long(const std::string& key){
    const std::string& val = get_value(key);
    if (!val.compare("")){
        //empty
        return 0;
    }

    try {
        return std::stol(val);
    } catch (const std::invalid_argument& ia){
        LWARN( "Config value " + key + " ( " + val + " ) must be an integer");
    } catch (const std::out_of_range& ia) {
        LWARN( "Config value " + key + " ( " + val + " ) is out of range ");
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
        LWARN( "Config value " + key + " ( " + val + " ) must be an integer");
    } catch (const std::out_of_range& ia) {
        LWARN( "Config value " + key + " ( " + val + " ) is out of range ");
    }
    
    return 0;

}

const std::string& Config::get_default_value(const std::string& key){
    for (auto& itr : setting_options){
        if (itr.key == key){\
            LDEBUG("Got default value of " + itr.def);
            return itr.def;
        }
    }
    LWARN("Code used undocumented config value '" + key + "'");
    return EMPTY_STR;
}
