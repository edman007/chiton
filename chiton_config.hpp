#ifndef __CHITON_CONFIG_HPP__
#define __CHITON_CONFIG_HPP__
#include <string>
#include <map>

class Config {
public:
    bool load_default_config();
    bool load_config(const std::string& path);

    
    const std::string get_value(const std::string& key);
    int get_value_int(const std::string& key);//returns the value as an int
    double get_value_double(const std::string& key);//returns the value as an double
    
    void set_value(const std::string& key, const std::string& value);

private:
    std::map<std::string,std::string> cfg_db;
};
#endif
