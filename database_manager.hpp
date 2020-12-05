#ifndef __DATABASE_MANAGER_HPP__
#define __DATABASE_MANAGER_HPP__
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
#include "database.hpp"

/*
 * Database Manager is called once at startup and prepares the database for use by updating it to
 * the latest version if required
 */
class DatabaseManager {
public:
    //initilize the database manager against the DB
    DatabaseManager(Database &db);

    //this queries the tables, determines the version, and initilizes the database as required
    //returns true if the database is in the correct configuration (and false if we cannot use the database)
    bool check_database(void);

private:
    Database &db;

    //returns true on success
    bool initilize_db(void);//initilized the database to the current version from an empty database

    bool set_latest_version(void);//marks the database version as the latest

    bool upgrade_database(int major, int minor);//selects the upgrade routine, returns true if successfully upgraded

    bool upgrade_from_1_0(void);//upgrade from 1_0 to latest
};

#endif
