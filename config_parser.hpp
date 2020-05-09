#ifndef __CONFIG_PARSER_HPP__
#define __CONFIG_PARSER_HPP__
#include "chiton_config.hpp"
#include <fstream>
#include <sstream>

class ConfigParser {
public:
    ConfigParser(Config& cfg, std::ifstream& ifs) : cfg(cfg), ifs(ifs) {};
    void parse(void);//reads the config and sets all values found into cfg

private:
    Config& cfg;
    std::ifstream& ifs;

    
    void reset_parser(void);
    
    char c;
    std::stringbuf key;
    std::stringbuf value;
    enum {
        BEFORE_KEY,
        IN_KEY,
        AFTER_KEY,
        BEFORE_VAL,
        IN_VAL,
        AFTER_VAL,
        IN_COMMENT
    } state;

    bool escape = false;//if we are escaping the next char
    bool doublequote = false;//we are inside double quotes
    bool singlequote = false;//we are inside single quotes

    
  
    //position info
    int line = 1;
    int pos = 0;

    bool next();//gets a character and updates position information

    void report_parse_error(void);
};

#endif
