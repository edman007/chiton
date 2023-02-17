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
 *   Copyright 2020-2023 Ed Martin <edman007@edman007.com>
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
    private $read_buf = "";

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
                break;//all data sent
            }
        }

        return true;
    }

    //requests that the backend performs a full reload
    public function reload_backend(){
        if (!$this->connect()){
            return false;
        }
        return $this->send_cmd('RELOAD');
    }

    //requests that the backend deletes the export
    public function rm_export($id){
        if (!$this->connect()){
            return false;
        }
        if ($this->send_cmd('RM-EXPORT ' . (int)$id)){
            return $this->get_next_line() == 'OK';
        } else {
            return false;
        }
    }

    //requests that the backend restarts the camera
    public function restart_cam($id){
        if (!$this->connect()){
            return false;
        }
        if ($this->send_cmd('RESTART ' . (int)$id)){
            $response = $this->get_next_line();
            if ($response == 'OK'){
                return true;
            } else {
                $this->error_msg = $response;
                return false;
            }
        } else {
            return false;
        }
    }

    //requests that the backend starts the camera
    public function start_cam($id){
        if (!$this->connect()){
            return false;
        }
        if ($this->send_cmd('START ' . (int)$id)){
            $response = $this->get_next_line();
            if ($response == 'OK'){
                return true;
            } else {
                $this->error_msg = $response;
                return false;
            }
        } else {
            return false;
        }
    }

    //requests that the backend stops the camera
    public function stop_cam($id){
        if (!$this->connect()){
            return false;
        }
        if ($this->send_cmd('STOP ' . (int)$id)){
            $response = $this->get_next_line();
            if ($response == 'OK'){
                return true;
            } else {
                $this->error_msg = $response;
                return false;
            }
        } else {
            return false;
        }
    }

    //requests the log messages from the camera
    public function log_cam($id){
        if (!$this->connect()){
            return false;
        }
        $ret = array();
        if ($this->send_cmd('LOG ' . (int)$id)){
            while (true){
                $response = $this->get_next_line();
                if ($response == 'OK'){
                    return $ret;
                } else if ($response == 'INVALID ARGUMENT') {
                    $this->error_msg = $response;
                    return false;
                } else {
                    $str_info = explode("\t", $response, 4);
                    if (count($str_info) != 4){
                        $this->error_msg = 'INVALID FORMAT';
                        return false;
                    }
                    $ret[] = array('id' => $str_info[0],
                                   'lvl' => $str_info[1],
                                   'time' => $str_info[2],
                                   'msg' => $str_info[3]);
                }
            }
        } else {
            return false;
        }
    }

    //requests the camera status
    public function list_status(){
        if (!$this->connect()){
            return false;
        }
        $ret = array();
        if ($this->send_cmd('LIST')){
            while (true){
                $response = $this->get_next_line();
                if ($response == 'OK'){
                    return $ret;
                } else {
                    $str_info = explode("\t", $response, 2);
                    if (count($str_info) != 2){
                        $this->error_msg = 'INVALID FORMAT';
                        return false;
                    }
                    $ret[$str_info[0]] = $str_info[1];
                }
            }
        } else {
            return false;
        }
    }

    public function get_error(){
        if (!empty($this->error_msg)){
            return $this->error_msg;
        } else {
            return socket_strerror(socket_last_error());
        }
    }

    private function get_next_line(){
        $end = strpos($this->read_buf, "\n");
        if ($end === FALSE){
            //read more data and call this again
            $data = socket_read($this->sock, 1024, PHP_NORMAL_READ);
            if ($data == ""){
                $data = $this->read_buf;
                $this->read_buf = "";
                return $data;
            } else {
                $this->read_buf .= $data;
                return $this->get_next_line();//try again
            }
        }
        //get the data up to end and delete the character at the end
        $data = substr($this->read_buf, 0, $end);
        $this->read_buf = substr($this->read_buf, $end + 1);
        return $data;
    }
  }
