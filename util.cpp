#include "util.hpp"

void Util::log_msg(const LOG_LEVEL lvl, const std::string& msg){
    if (lvl == LOG_ERROR){
        std::cerr << msg << std::endl;
    } else {
        std::cout << msg << std::endl;
    }
}
