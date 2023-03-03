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
 *   Copyright 2023 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "json_encoder.hpp"
#include <sstream>

JSONEncoder::JSONEncoder(){
    valid = false;
}


JSONEncoder::~JSONEncoder(){
    //nothing
}


bool JSONEncoder::add(const std::string key, int val){
    data[key] = std::to_string(val);
    valid = false;
    return true;
}

bool JSONEncoder::add(const std::string key, unsigned int val){
    data[key] = std::to_string(val);
    valid = false;
    return true;
}

bool JSONEncoder::add(const std::string key, time_t val){
    add(key, (unsigned int)val);//should this be unsigned long?
    return true;
}

bool JSONEncoder::add(const std::string key, double val){
    data[key] = std::to_string(val);
    valid = false;
    return true;
}

bool JSONEncoder::add(const std::string key, bool val){
    if (val){
        data[key] = "true";
    } else {
        data[key] = "false";
    }
    valid = false;
    return true;
}

bool JSONEncoder::add(const std::string key, const char* val){
    std::string str = std::string(val);
    return add(key, str);
}

bool JSONEncoder::add(const std::string key, const std::string &val){
    //we only bother escaping " and \ as they affect formatting
    const std::string esc_chars = "\"\\";//"\"\\\b\f\n\r\t";
    std::ostringstream str;
    str << '"';
    size_t pos = 0;//the start position
    do {
        //find the next escapable character
        size_t end = val.find_first_of(esc_chars, pos);
        if (end == std::string::npos){
            //the end
            str << val.substr(pos);
            pos = end;
        } else {
            str << val.substr(pos, end - pos);
            //escape this character
            switch (val[end]){
            case '"':
                str << "\\\"";
                break;
            case '\\':
                str << "\\\\";
                break;
            default:
                str << val[end];//error..should never get here
            }
            pos = end+1;
        }

    } while (pos < val.length());
    str << '"';
    data[key] = str.str();
    valid = false;
    return true;
}

bool JSONEncoder::add(const std::string key, JSONEncoder &val){
    data[key] = val.str();
    valid = false;
    return true;
}

const std::string& JSONEncoder::str(void){
    if (valid){
        return output;
    }
    std::stringstream s;
    s << "{";
    for (auto it = data.begin(); it != data.end(); it++) {
        s << "\"" << it->first << "\":" << it->second;
            s << ",";
    }
    //delete last , and replace with }
    s.seekp(-1, std::ios_base::end);
    s << "}";
    output = s.str();
    return output;
}
