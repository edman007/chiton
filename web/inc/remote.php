<?php
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

 /*
  * This class opens a connection to the backend and sends various commands
  */
class Remote {
    private $db;
    private $sock;
    private $error_msg = null;

    function __construct($db){
        $this->db = $db;
        $this->sock = false;
    }

    function __destruct(){
        if ($this->sock !== false){
            $this->send_cmd('CLOSE');
            socket_close($this->sock);
            $this->sock = false;
        }
    }

    //returns true if we can connect to the server
    public function connect(){
        global $cfg;
        if ($this->sock !== false){
            return true;
        }

        $path = $cfg->get_value('socket-path');
        if (empty($path)){
            $this->error_msg = 'socket-path is empty';
            return false;
        }

        $this->sock = socket_create(AF_UNIX, SOCK_STREAM, 0);
        if ($this->sock === false){
            return false;
        }

        if (socket_connect($this->sock, $path)){
            return true;
        } else {
            socket_close($this->sock);
            $this->sock = false;
        }
    }

    //sends the specified command to the backend
    private function send_cmd($cmd){
        $cmd .= "\n";
        while (strlen($cmd) > 0){
            $len = socket_write($this->sock, $cmd, strlen($cmd));
            if ($len === false){
                return false;
            } else if (strlen($cmd) > $len){
                //cut the cmd up and try again
                $cmd = substr($cmd, $len);
            } else {
                //all data was sent
                return true;
            }
        }
        return true;//should never actually get here
    }

    //requests that the backend performs a full reload
    public function reload_backend(){
        if (!$this->connect()){
            return false;
        }
        return $this->send_cmd('RELOAD');
    }

    public function get_error(){
        if (!empty($this->error_msg)){
            return $this->error_msg;
        } else {
            return socket_strerror(socket_last_error());
        }
    }

  }
