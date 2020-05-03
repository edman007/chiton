#include "util.hpp"

void Util::log_msg(std::string msg, LOG_LEVEL lvl){
    if (lvl == LOG_ERROR){
        std::cerr << msg << std::endl;
    } else {
        std::cout << msg << std::endl;
    }
}
