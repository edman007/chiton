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
#include "config_parser.hpp"
#include "util.hpp"

void ConfigParser::parse(void){
    reset_parser();
    
    while (next()){
        if ((state == IN_COMMENT || state == AFTER_VAL || state == BEFORE_VAL) && c == '\n'){
            reset_parser();
            continue;//no use in continuing if we are in a comment
        }
        if (c == '\n' && (state == IN_KEY || state == BEFORE_VAL || state == AFTER_VAL)){
            report_parse_error();
            return;
        }
        if (c == '\n' && !doublequote && !singlequote && state == IN_VAL){
            //set the value
            cfg.set_value(key.str(), value.str());
            reset_parser();
            continue;
        }

        if (c == '\n' && !doublequote && !singlequote && state == BEFORE_KEY){
            reset_parser();
            continue;
        }

        //check for the comment
        if (!doublequote && !singlequote && c == '#'){
            //starting a comment...
            if (state == IN_KEY ||
                state == AFTER_KEY ||
                state == BEFORE_VAL){
                //invalid
                report_parse_error();
                return;
            }
            if (state == IN_VAL && !doublequote && !singlequote){
                //set the value
                cfg.set_value(key.str(), value.str());
                state = IN_COMMENT;
                continue;
            }
            state = IN_COMMENT;
            continue;

        }
        switch (state){
            
        case BEFORE_KEY:
            if (c != ' ' && c != '\t'){
                key.sputc(c);
                state = IN_KEY;
            }
            break;
            
        case IN_KEY:
            if (c == '=') {
                state = BEFORE_VAL;
            } else if (c == ' ' || c == '\t'){
                state = AFTER_KEY;
            } else {
                key.sputc(c);
            }
            break;
            
        case AFTER_KEY:
            if (c == '='){
                state = BEFORE_VAL;
            }
            break;
            
        case BEFORE_VAL:
            if (c == '\''){
                singlequote = true;
                state = IN_VAL;
            } else if (c == '"'){
                doublequote = true;
                state = IN_VAL;
            } else if (c != ' ' && c != '\t'){
                value.sputc(c);
                state = IN_VAL;
            }
            break;
            
        case IN_VAL:
            if (c == '\\' && (singlequote || doublequote )){
                if (escape){
                    value.sputc('\\');
                    escape = false;
                    continue;
                } else {
                    escape = true;
                    continue;
                }
                    
            }

            if (c == '"' && escape && doublequote){
                value.sputc('"');
                escape = false;
                continue;
            } else if (c == '"' && doublequote){
                //we are done
                cfg.set_value(key.str(), value.str());
                state = AFTER_VAL;
                continue;
            }

            if (c == '\'' && escape && singlequote){
                value.sputc('\'');
                escape = false;
                continue;
            } else if (c == '\'' && singlequote){
                //we are done
                cfg.set_value(key.str(), value.str());
                state = AFTER_VAL;
                continue;
            }

            if ((c == ' ' || c == '\t') && (singlequote || doublequote)){
                //we are done
                cfg.set_value(key.str(), value.str());
                state = AFTER_VAL;
                continue;
            }

            if (escape){
                //nothing else is escapable, so we add the \ back in
                value.sputc('\\');
                escape = false;
                //and continue, because we are on a new character...
            }

            value.sputc(c);
            break;
            
        case AFTER_VAL:
            //anything after the value might as well be a comment
            if (c == '#'){
                state = IN_COMMENT;
            }
            
        case IN_COMMENT:
            //it's a comment, nothing to do
            break;
            
        }
    }
    if (state == IN_VAL && !singlequote && !doublequote){
        //end of file in a value means we are done
        cfg.set_value(key.str(), value.str());
    } else if (singlequote || doublequote || (state != BEFORE_KEY && state != AFTER_VAL && state != IN_COMMENT && state != IN_VAL)){
        //file ended in the middle of parsing
        report_parse_error();
    }
}

void ConfigParser::reset_parser(void){
    //the default for the start of a line:
    key.str("");
    value.str("");
    state = BEFORE_KEY;
    escape = false;
    doublequote = false;
    singlequote = false;
}

bool ConfigParser::next(void){
    ifs.get(c);
    pos++;
    if (c == '\n'){
        line++;
        pos = 0;
    }
    return ifs.good();
}

void ConfigParser::report_parse_error(void){
    LWARN( "Parse error in config file, line " + std::to_string(line) + ", char " + std::to_string(pos));
}
