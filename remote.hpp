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
 *   Copyright 2020-2023 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "chiton_config.hpp"
#include "system_controller.fwd.hpp"
#include "export.hpp"
#include "database.hpp"
#include <atomic>
#include <thread>
#include <list>
#include <map>
#include <vector>
/*
 * This class creates a socket to listen for commands from the web interface
 * It will spawn off a worker thread that listens for and handles actions requested
 * from the web frontend
 */


/*
 * Definition of socket protocal:
 *
 * Commands are all CAPS, and case sensitive
 * The client sends commands, seperated by \n, weather or not the server responds depends on the command
 * The connection should be closed by calling CLOSE before disconnecting
 *
 * A typical connection looks like this
 *
 *   -> RELOAD
 *   <- OK
 *   -> CLOSE
 *
 * Command Documentation:
 *
 * RELOAD
 *  - Server will respond with "OK" when processed correctly
 *  - This will reload the backend, as if the HUP signal was sent
 *
 * RM-EXPORT <INT1>
 *  - Server will delete the export with the export ID <INT1>
 *  - Server will cancel the export in progress if this id is currently being exported
 *
 * CLOSE
 *  - Server responds by closing the connection
 *
 * LICENSE
 *  - Prints the license
 *
 * TEAPOT
 *  - Prints I'm a teapot
 *
 * HELP
 *  - Server responds with 'SUPPORTED COMMANDS:' followed by a space deliminated list of commands supported
 *
 * START <INT1>
 *  - Requests that camera with ID <INT1> is started
 *  - Invalid or inactive camera IDs do nothing
 *  - Prints "OK" if added, "BUSY" if already queued for starting
 *
 * STOP <INT1>
 *  - Requests that camera with ID <INT1> is started
 *  - If no camera with this ID is running then does nothing
 *  - Prints "OK" if added, "BUSY" if already queued for stopping
 *
 * RESTART <INT1>
 *  - Requests that camera with ID <INT1> is started
 *  - Equivilent to running STOP and then START atomically
 *  - Prints "OK" if added, "BUSY" if already queued for starting
 *
 * LOG <INT1>
 *  - Print the log output for given camera, one message per line, use -1 as the camera to access system messages
 *  - Return value is "<INT1>\t<INT2>\t<INT3>\t<STR>", INT1 is the camera ID, INT2 is the message level, INT3 is seconds since UTC, STR is the message
 *  - output is terminated by OK by itself on a line
 *
 * LIST
 *  - List all running cameras
 *  - response is <INT1>\t<STATUS>
 *    INT1 - ID
 *    STATUS - The current state, STARTING, STOPPING, RESTARTING, RUNNING
 *  - Terminated by "OK" on it's own line
 *
 * STATUS <INT1>
 *  - Show status of camera INT1 (returned as JSON)
 *  - response is <INT2> on one line, follwed by a \n followed by a JSON string of INT2 len, followed by \nOK\n
 */
class Remote {
public:
    Remote(SystemController &sys);
    ~Remote();

    void init(void);//opens the socket
    void shutdown(void);//closes the socket

    //returns if a reload was requested an clears any pending request
    bool get_reload_request(void);

private:
    SystemController &sys;
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

    //the callback below calls a member function to process the command, with the args: fd, rc (this rc struct), cmd (the full line being processed)
    struct RemoteCommand;
    std::vector<RemoteCommand> command_vec;

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

    //the methods to process each command:
    void cmd_reload(int fd, RemoteCommand& rc, std::string &cmd);
    void cmd_rm_export(int fd, RemoteCommand& rc, std::string &cmd);
    void cmd_close(int fd, RemoteCommand& rc, std::string &cmd);
    void cmd_help(int fd, RemoteCommand& rc, std::string &cmd);
    void cmd_license(int fd, RemoteCommand& rc, std::string &cmd);
    void cmd_teapot(int fd, RemoteCommand& rc, std::string &cmd);
    void cmd_start(int fd, RemoteCommand& rc, std::string &cmd);
    void cmd_stop(int fd, RemoteCommand& rc, std::string &cmd);
    void cmd_restart(int fd, RemoteCommand& rc, std::string &cmd);
    void cmd_log(int fd, RemoteCommand& rc, std::string &cmd);
    void cmd_list(int fd, RemoteCommand& rc, std::string &cmd);
    void cmd_status(int fd, RemoteCommand& rc, std::string &cmd);
};

#endif
