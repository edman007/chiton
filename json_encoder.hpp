#ifndef __JSON_ENCODER_HPP__
#define __JSON_ENCODER_HPP__
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

#include "chiton_config.hpp"
#include <string>
#include <map>
#include <vector>

//intended to be solely a wrapper to convert data to a json string
class JSONEncoder {
public:
    JSONEncoder();
    ~JSONEncoder();

    const std::string& str(void);//return the object as a string

    //these are the setters
    bool add(const std::string key, int val);
    bool add(const std::string key, unsigned int val);
    bool add(const std::string key, double val);
    bool add(const std::string key, bool val);
    bool add(const std::string key, const std::string &val);
    bool add(const std::string key, const char* val);
    bool add(const std::string key, JSONEncoder &val);
    //array is unsupported so far, will implement when required
private:
    std::string output;//output string
    bool valid;//true if output is fully generated
    std::map<std::string, std::string> data;
};
#endif
