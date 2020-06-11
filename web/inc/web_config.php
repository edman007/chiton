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
    function __construct($db){
        $this->db = $db;
        $sql = 'SELECT name, value FROM config WHERE camera = -1';
        $res = $db->query($sql);
        if ($res){
            while ($row = $res->fetch_assoc()){
                $this->settings[$row['name']] = $row['value'];
            }
        }
    }

    function get_value($name){
        if (!isset($this->settings[$name])){
            return "";
        }
        return $this->settings[$name];
    }
    
}

?>
