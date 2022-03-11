#ifndef __MODULE_HPP__
#define __MODULE_HPP__
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
template <class Mod, class Controller> class Module;
#include "module_controller.hpp"

//this is the base Module that all modules must implement
//Mod is a class derived from Module<> and Controller is a class derived from a ModuleController<>
template <class Mod, class Controller> class Module {
public:
    Module(Config &cfg, Database &db, Controller &controller, const std::string &name) : cfg(cfg), db(db), controller(controller), name(name) {};
    virtual ~Module() {};
    const std::string& get_name(void) const { return name; };//return the name of the algorithm
    static const std::string& get_mod_name(void) { return "unknown"; };//return the name of the algorithm (statically)
    virtual bool init(void) {return true;};//called immeditly after the constructor to allow dependicies to be setup
protected:
    Config &cfg;
    Database &db;
    Controller &controller;
    std::string name;
};


//this is used by the controller to allocate the module (T=DerivedModule, V=DerivedModuleController
template <class T, class U>  class ModuleAllocator {
public:
    virtual ~ModuleAllocator() {};
    virtual T* allocate(Config &cfg, Database &db, U &controller) = 0;//allocate a module, ModuleFactory provides an implementation
    virtual const std::string& get_name(void) = 0;//return the name of the module, ModuleFactory provides an implementation
};

//use this to register a new module with the controller
//T=ModuleClass, U=ModuleBase, V=moduleController
template <class T, class U, class V> class ModuleFactory : public ModuleAllocator<U, V> {
    U* allocate(Config &cfg, Database &db, V &controller) { return new T(cfg, db, controller); };//constructs a new Module with given base and controller
    const std::string& get_name(void) { return T::get_mod_name(); };//returns the name of the referenced Module with given base and controller
};

#endif
