#ifndef __MODULE_CONTROLLER_HPP__
#define __MODULE_CONTROLLER_HPP__
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
 *   Copyright 2022 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "config_build.hpp"
#include "chiton_ffmpeg.hpp"
#include "chiton_config.hpp"
#include "util.hpp"
#include <vector>
template <class Mod, class Controller> class ModuleController;
#include "module.hpp"

//this class provides a generic way of registering modules
//Mod is a class derived from Module<> and Controller is a class derived from a ModuleController<>
template <class Mod, class Controller> class ModuleController {
public:
    //name is the name of the controller, name-mods is queried for the list of modules
    ModuleController(Config &cfg, Database &db, const std::string name);
    ~ModuleController();

    //returns the event notification modrithm with name, will be executed before mod, returns null if the modrithm does not exist, calls init() on the modrithm
    Mod* get_module_before(const std::string &name, const Mod *mod);
    void register_module(ModuleAllocator<Mod, Controller>* maa);//registers an allocator and makes an modrithm available for use
    const std::string& get_name(void);//return the name of the module controller;

protected:
    Config &cfg;
    Database &db;
    std::vector<ModuleAllocator<Mod, Controller>*> supported_mods;//list of allocators supported
    std::vector<Mod*> mods;//list of configured modules

    bool add_mods(void);//put all mods in mods
private:
    std::string name;//the name of the modulecontroller
    void clear_mods(void);//clear & delete mods
    bool parse_mods(const std::string param, std::vector<std::string> &mod_list);//read the config value param, write out all modrithms to mod_list, return true if non-empty
    Mod* find_mod(const std::string &name);//return the mod with a given name, null if it does not exist, allocate it if not already done
};


template <class Mod, class Controller> ModuleController<Mod, Controller>::ModuleController(Config &cfg, Database &db, const std::string name) : cfg(cfg), db(db), name(name) {

}

template <class Mod, class Controller> ModuleController<Mod, Controller>::~ModuleController(){
    clear_mods();
    for (auto &m : supported_mods){
        delete m;
    }

}
template <class Mod, class Controller> Mod* ModuleController<Mod, Controller>::get_module_before(const std::string &mod_name, const Mod* mod){
    for (auto it = mods.begin(); it != mods.end(); it++){
        if (*it == mod){
            for (auto &m : supported_mods){
                if (m->get_name() == mod_name){
                    Mod* new_mod = m->allocate(cfg, db, *static_cast<Controller*>(this));
                    mods.insert(it, new_mod);
                    new_mod->init();
                    return new_mod;
                }
            }
            return NULL;//not found
        }
        if ((*it)->get_name() == mod_name){
            return *it;
        }
    }
    return NULL;//mod was not found

}
template <class Mod, class Controller> void ModuleController<Mod, Controller>::register_module(ModuleAllocator<Mod, Controller>* ma) {
    if (ma){
        supported_mods.push_back(ma);
    }
}

template <class Mod, class Controller> void ModuleController<Mod, Controller>::clear_mods(void){
    for (auto &m : mods){
        delete m;
    }
    mods.clear();
}

template <class Mod, class Controller> bool ModuleController<Mod, Controller>::parse_mods(const std::string param, std::vector<std::string> &mod_list){
    const std::string &str_list = cfg.get_value(param);
    std::string::size_type start = 0;
    while (start < str_list.length()){
        auto end = str_list.find(",", start);
        if (end == start){
            start++;
            continue;
        }
        mod_list.push_back(str_list.substr(start, end));
        if (end == std::string::npos){
            break;
        }
        start = end + 1;
    }
    return !mod_list.empty();
}

template <class Mod, class Controller> Mod* ModuleController<Mod, Controller>::find_mod(const std::string &mod_name){
    for (auto &m : mods){
        if (m->get_name() == mod_name){
            return m;
        }
    }
    for (auto &m : supported_mods){
        if (m->get_name() == mod_name){
            Mod* new_mod = m->allocate(cfg, db, *static_cast<Controller*>(this));
            mods.push_back(new_mod);
            mods.back()->init();
            return mods.back();
        }
    }
    return NULL;

}
template <class Mod, class Controller> bool ModuleController<Mod, Controller>::add_mods(void){
    std::vector<std::string> mod_list;
    if (!parse_mods(name + "-mods", mod_list)){
        return true;//nothing to configure
    }
    if (mod_list[0] == "" || mod_list[0] == "none"){
        return true;//explicitly disabled
    }

    bool ret = false;
    for (auto &mod_name : mod_list){
        auto m = find_mod(mod_name);
        if (m != NULL){
            ret |= true;
        } else {
            LWARN("Cannot find module '" + mod_name + "' in '" +  + "', skipping");
        }
    }

    return true;
}

#endif
