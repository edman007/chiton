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


#include "remote.hpp"
#include "util.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <functional>

const int socket_backlog = 5;//maxiumn connection backlog for the socket
struct Remote::RemoteCommand {const std::string cmd;
        std::function<void(Remote&, int, RemoteCommand&, std::string&)> cbk;
        };

Remote::Remote(Database &db, Config &cfg, Export &expt) : db(db), cfg(cfg), expt(expt) {
    sockfd = -1;
    killfd_worker = -1;
    killfd_master = -1;
    reload_requested = false;

    //fill the supproted commands
    command_vec.push_back({"RELOAD", &Remote::cmd_reload});
    command_vec.push_back({"RM-EXPORT", &Remote::cmd_rm_export});
    command_vec.push_back({"CLOSE", &Remote::cmd_close});
    command_vec.push_back({"HELP", &Remote::cmd_help});
    command_vec.push_back({"LICENSE", &Remote::cmd_license});
    command_vec.push_back({"TEAPOT", &Remote::cmd_teapot});

    if (create_socket()){
        spawn_worker();
    }
}

Remote::~Remote(){
    stop_worker();
    if (worker.joinable()){
        //force the worker to exit...how?
        worker.join();
    }
    if (sockfd != -1){
        //shutdown the workers...
        close(sockfd);
        if (unlink(path.c_str())){
            LWARN("unlink of " + path + " failed " + std::to_string(errno));
        }
        sockfd = -1;
    }

    //kill all open connections
    for (auto it = conn_list.begin(); it != conn_list.end(); ){
        auto next_it = it;
        next_it++;//grab a reference to the next it
        close_conn(*it);
        it = next_it;//this is valid, old it is not valid
    }
}

bool Remote::get_reload_request(void){
    return reload_requested.exchange(false);
}


bool Remote::create_socket(void){
    struct sockaddr_un sockaddr;
    path = cfg.get_value("socket-path");

    //attempt to create the socket
    if (path.length() == 0){
        LWARN("socket-path is empty, will not create socket");
        return false;
    }

    if (path.length() > (sizeof(sockaddr.sun_path) - 1)){
        LWARN("Path to socket is too long");
        return false;
    }


    //check if the socket exists, and if so delete it, if it exists but it's not a socket bail
    struct stat statbuf;
    if (!stat(path.c_str(), &statbuf)){
        if (S_ISSOCK(statbuf.st_mode)) {
            if (unlink(path.c_str())){
                LWARN("Could not delete " + path + " (" + std::to_string(errno) + ")");
                return false;
            }
        } else {
            LWARN(path + " exists and it's not a socket");
            return false;
        }
    }//we proceed if stat fails, bind will fail later if it's a problem

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1){
        LWARN("Creation of UNIX socket failed " + std::to_string(errno));
        return false;
    }

    auto len = path.copy(sockaddr.sun_path, sizeof(sockaddr.sun_path) - 1, 0);
    sockaddr.sun_path[len] = '\0';
    sockaddr.sun_family = AF_UNIX;

    if (bind(sockfd, (const struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1){
        LWARN("Binding socket " + path +" failed " + std::to_string(errno));
        close(sockfd);
        sockfd = -1;
        return false;
    }

    //chmod it, need to restrict permissions as we currently use this as out only security
    if (chmod(path.c_str(), 0660)){
        LWARN("chmod of socket failed " + std::to_string(errno));
        close(sockfd);
        sockfd = -1;
        unlink(path.c_str());
        return false;
    }

    if  (listen(sockfd, socket_backlog)){
        LWARN("listen on socket failed " + std::to_string(errno));
        close(sockfd);
        sockfd = -1;
        unlink(path.c_str());
        return false;
    }

    //make a pipe
    int pipefd[2];
    if (pipe(pipefd)){
        LWARN("Failed to make a pipe " + std::to_string(errno));
        close(sockfd);
        unlink(path.c_str());
        return false;
    }
    killfd_master = pipefd[1];
    killfd_worker = pipefd[0];

    LINFO("Socket listening on " + path);
    return true;
}

bool Remote::spawn_worker(void){
    kill_worker = false;
    try {
        worker = std::thread(&Remote::run_worker, this);
    } catch (const std::system_error& e){
        LWARN("System error starting worker socket thread " + std::string(e.what()));
        return false;
    }
    return true;
}

void Remote::run_worker(void){

    //main loop
    do {
        int count = wait_for_activity();
        if (count > 0){
            if (FD_ISSET(kill_worker, &read_fds)){
                continue;//we need to exit
            }
            if (FD_ISSET(sockfd, &read_fds)){
                //open a new connection
                LDEBUG("Accepting socket connection");
                accept_new_connection();
            }
            for (auto fd_it = conn_list.begin(); fd_it != conn_list.end();){
                auto fd_next = fd_it;//we do this because the below methods may invalidate the iterator
                fd_next++;

                int fd = *fd_it;
                if (FD_ISSET(fd, &write_fds)){
                    complete_write(fd);
                } else if (FD_ISSET(fd, &read_fds)){
                    process_input(fd);
                }

                fd_it = fd_next;
            }

        }
    } while (!kill_worker);
}


int Remote::wait_for_activity(void){
    nfds = 0;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    FD_SET(sockfd, &read_fds);
    nfds = std::max(nfds, sockfd);

    FD_SET(killfd_worker, &read_fds);
    nfds = std::max(nfds, killfd_worker);

    for (auto fd : conn_list){
        FD_SET(fd, &read_fds);
        nfds = std::max(nfds, fd);
        if (write_buffers.find(fd) != write_buffers.end()){
            FD_SET(fd, &write_fds);
        }
    }

    return select(nfds + 1, &read_fds, &write_fds, NULL, NULL);
}

void Remote::stop_worker(void){
    kill_worker = true;
    if (killfd_master == -1){
        return;//no worker to kill
    }

    int ret;
    do {
        ret = write(killfd_master, "x", 2);
        if (ret < 1){
            if (errno == EAGAIN || errno == EINTR){
                continue;
            } else {
                LWARN("Error sending signal to kill socket worker " + std::to_string(errno));
                return;
            }
        }
    } while (ret <= 0);
}


void Remote::accept_new_connection(void){
    int new_fd = accept(sockfd, NULL, NULL);
    if (new_fd >= 0){
        conn_list.insert(conn_list.end(), new_fd);
    }
}

void Remote::complete_write(int fd){
    auto str = write_buffers.find(fd);
    if (str == write_buffers.end()){
        return;//doesn't exist
    }
    int ret = write(fd, str->second.c_str(), str->second.length());
    if ((unsigned int)ret != str->second.length() && ret >= 0){
        //write didn't complete
        std::string new_buf = str->second.substr(ret, std::string::npos);
        write_buffers[fd] = new_buf;
    } else if (ret < 0 && errno != EAGAIN && errno != EINTR){
        //fatal error
        LWARN("Write to socket connection failed " + std::to_string(errno));
        close_conn(fd);
    } else {
        //good write
        write_buffers.erase(fd);
    }
}


void Remote::write_data(int fd, std::string str){
    auto target = write_buffers.find(fd);
    if (target == write_buffers.end()){
        write_buffers[fd] = str;
    } else {
        write_buffers[fd] = write_buffers[fd] + str;
    }
    complete_write(fd);
}

void Remote::process_input(int fd){
    char buf[1024];
    int cnt = sizeof(buf);
    cnt = read(fd, buf, cnt);
    if (cnt <= 0){
        if (cnt < 0 && errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK){
            LWARN("Read of socket connection failed " + std::to_string(errno));
            close_conn(fd);
        } else if (cnt == 0 && errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK){
            close_conn(fd);//the connection has been closed
        }
        return;
    }

    //add the new buffer to the input buffer
    auto sbuf = read_buffers.find(fd);
    if (sbuf == read_buffers.end()){
        read_buffers[fd] = std::string(std::string(buf, cnt));
    } else {
        read_buffers[fd] = std::string(read_buffers[fd] + std::string(buf, cnt));
    }

    //now we get the lines and process them
    std::string::size_type pos = 0;
    std::string::size_type newline = 0;
    do {
        //check if a newline exists
        newline = read_buffers[fd].find('\n', pos);

        if (newline != std::string::npos){
            //good data, extract it and  process
            auto cmd = read_buffers[fd].substr(pos, newline - pos);
            execute_cmd(fd, cmd);
            pos = newline + 1;
        } else if ((read_buffers[fd].length() - pos) > 1024){
            LWARN("Socket connection received line too long");
            close_conn(fd);
            return;
        }
    } while (pos < read_buffers[fd].length() && newline != std::string::npos);

    //now delete the bits of the buffer that have been read
    if (pos == read_buffers[fd].length()){
        read_buffers.erase(fd);
    } else if (newline == std::string::npos) {//we had some data left
        read_buffers[fd] = read_buffers[fd].substr(pos);
    }

}

void Remote::close_conn(int fd){
    if (fd < 0){
        return;
    }
    LDEBUG("Closing Socket Connection");
    close(fd);

    write_buffers.erase(fd);

    for (auto it = conn_list.begin(); it != conn_list.end(); it++){
        if (*it == fd){
            conn_list.erase(it);
            break;
        }
    }
}

void Remote::execute_cmd(int fd, std::string &cmd){
    for (auto rc : command_vec){
        if (cmd.find(rc.cmd) == 0){
            LDEBUG("Calling " + rc.cmd);
            rc.cbk(*this, fd, rc, cmd);
            return;
        }
    }
    write_data(fd, "Unknown Command: " + cmd +"\n");
}

void Remote::cmd_reload(int fd, RemoteCommand& rc, std::string &cmd){
    LINFO("Reload Requested via socket");
    reload_requested.exchange(true);
    write_data(fd, "OK\n");
}

void Remote::cmd_rm_export(int fd, RemoteCommand& rc, std::string &cmd){
    int export_id = std::stoi(cmd.substr(rc.cmd.length() + 1));
    if (expt.rm_export(export_id)){
        write_data(fd, "OK\n");
    } else {
        write_data(fd, "ERROR\n");
    }
}

void Remote::cmd_close(int fd, RemoteCommand& rc, std::string &cmd){
    close_conn(fd);
}

void Remote::cmd_help(int fd, RemoteCommand& rc, std::string &cmd){
    std::string supported_commands = "";
    for (auto rc : command_vec){
        supported_commands += " " + rc.cmd;
    }
    write_data(fd, "SUPPORTED COMMANDS:" + supported_commands + "\n");
}


void Remote::cmd_license(int fd, RemoteCommand& rc, std::string &cmd){
    write_data(fd, "chiton Copyright (C) 2020-2022  Ed Martin <edman007@edman007.com> \n"
               "This program is free software: you can redistribute it and/or modify "
               "it under the terms of the GNU General Public License as published by "
               "the Free Software Foundation, either version 3 of the License, or "
               "(at your option) any later version. "
               "This program is distributed in the hope that it will be useful, "
               "but WITHOUT ANY WARRANTY; without even the implied warranty of "
               "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
               "GNU General Public License for more details. "
               "You should have received a copy of the GNU General Public License "
               "along with this program.  If not, see <https://www.gnu.org/licenses/>.\n");
}

void Remote::cmd_teapot(int fd, RemoteCommand& rc, std::string &cmd){
    write_data(fd, "I'm a teapot! 29b6b3dad9327e74f5bff8259f9965a5\n");//echo -n edman007''@''edman007.com | md5sum  -
}
