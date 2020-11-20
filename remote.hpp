#ifndef __REMOTE_HPP__
#define __REMOTE_HPP__
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

#include "chiton_config.hpp"
#include "database.hpp"
#include <atomic>
#include <thread>
#include <list>
#include <map>

/*
 * This class creates a socket to listen for commands from the web interface
 * It will spawn off a worker thread that listens for and handles actions requested
 * from the web frontend
 */
class Remote {
public:
    Remote(Database &db, Config &cfg);
    ~Remote();

    //returns if a reload was requested an clears any pending request
    bool get_reload_request(void);

private:
    Database &db;
    Config &cfg;
    std::atomic_bool reload_requested;
    int sockfd;
    std::string path;
    std::thread worker;
    std::atomic_bool kill_worker;
    int killfd_worker;
    int killfd_master;

    //for the select() call
    int nfds;
    fd_set read_fds;
    fd_set write_fds;

    std::list<int> conn_list;//list of connections we have
    std::map<int,std::string> write_buffers;//unwritten responses, indexed by fd
    std::map<int, std::string> read_buffers;

    bool create_socket(void);//create the socket and bind to it
    bool spawn_worker(void);
    void run_worker(void);
    int wait_for_activity(void);//calls select() on all open fds, returns the number of FDs that are ready
    void accept_new_connection(void);//accepts one new connection
    void complete_write(int fd);//writes out any remaining buffer to the fd
    void write_data(int fd, std::string str);//writes out string to the fd
    void process_input(int fd);
    void execute_cmd(int fd, std::string &cmd);//executes the command received
    void stop_worker(void);//sends the shutdown signal to the worker
    void close_conn(int fd);//close the connection associated with fd and any associated data
};

#endif
