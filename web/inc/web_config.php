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

class WebConfig {
    private $settings;
    private $db;
    private $camera_cfg;
    function __construct($db, $camera = -1){
        $this->db = $db;
        $this->camera_cfg = $camera != -1;
        $sql = 'SELECT name, value FROM config WHERE camera = ' . (int)$camera;
        $res = $db->query($sql);
        if ($res){
            while ($row = $res->fetch_assoc()){
                $this->settings[$row['name']] = $row['value'];
            }
        }
    }

    function get_value($name){
        global $cfg;
        if (!isset($this->settings[$name])){
            if ($this->camera_cfg){
                return $cfg->get_value($name);
            } else {
                return $this->get_default_value($name);
            }
        }
        return $this->settings[$name];
    }

    //returns if the value is from the database and not pulled from system defaults
    function isset($name){
        return isset($this->settings[$name]);
    }
    
    //gets the default value of an item
    function get_default_value($name){
        return $this->get_default_info($name)[0];
    }

    //get the short description for this value
    function get_short_desc($name){
        return $this->get_default_info($name)[1];
    }

    //get the description for this
    function get_desc($name){
        return $this->get_default_info($name)[2];
    }

    //return VALUE is one of SETTING_READ_ONLY, SETTING_REQUIRED_SYSTEM, SETTING_OPTIONAL_SYSTEM, SETTING_REQUIRED_CAMERA, SETTING_OPTIONAL_CAMERA
    function get_priority($name){
        return $this->get_default_info($name)[3];
    }

    //returns the array of info for the config parameter
    function get_default_info($name){
        global $default_settings;
        return $default_settings[$name];//it is an error if this is not set
    }

    //returns if this is a value we can modify
    function issettable($name){
        global $default_settings;
        if (isset($default_settings[$name])){
            return $this->get_priority($name) != SETTING_READ_ONLY;
        }
        return false;
    }
    //gets the info for all config of type
    function get_defaults_of_type($type){
        global $default_settings;
        $ret = array();
        foreach ($default_settings as $info){
            if ($info[3] == $type){
                $ret[] = $info;
            }
        }
        return $ret;
    }

    //gets an array of all possible keys
    function get_all_keys(){
        global $default_settings;
        return array_keys($default_settings);
    }
}
