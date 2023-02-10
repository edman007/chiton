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

require_once('./inc/main.php');

$smarty->assign('title', 'Settings');
$camera_id = -1;
$system_messages = array();
if (isset($_GET['camera'])){
    $camera_id = (int)$_GET['camera'];
}

if ($_SERVER['REQUEST_METHOD'] == 'POST'){
    //update the DB...
    
    if (!empty($_POST['delete_camera']) && isset($_POST['camera_id'])){
        //delete this camera
        $camera = (int)$_POST['camera_id'];
        if ($camera >= 0){
            $sql = 'DELETE FROM config WHERE camera = ' . $camera;
            $db->query($sql);
            $system_messages[] = 'Camera Deleted';
        }
    } else if (!empty($_POST['reload_backend'])){
        require_once('./inc/remote.php');
        $remote = new Remote($db);
        if ($remote->reload_backend()){
            $system_messages[] = 'Backend Reloaded';
        } else {
            //reload failed
            $system_messages[] = 'Reloading the backend failed ' . $remote->get_error();
        }
    } else if (!empty($_POST['restart_cam']) && isset($_POST['camera_id'])){
        require_once('./inc/remote.php');
        $remote = new Remote($db);
        $camera = (int)$_POST['camera_id'];
        if ($remote->restart_cam($camera)){
            $system_messages[] = 'Camera Restarted';
        } else {
            //restart failed
            $system_messages[] = 'Restarting Camera failed ' . $remote->get_error();
        }
    } else if (!empty($_POST['start_cam']) && isset($_POST['camera_id'])){
        require_once('./inc/remote.php');
        $remote = new Remote($db);
        $camera = (int)$_POST['camera_id'];
        if ($remote->start_cam($camera)){
            $system_messages[] = 'Camera Started';
        } else {
            //restart failed
            $system_messages[] = 'Starting Camera failed ' . $remote->get_error();
        }
    } else if (!empty($_POST['stop_cam']) && isset($_POST['camera_id'])){
        require_once('./inc/remote.php');
        $remote = new Remote($db);
        $camera = (int)$_POST['camera_id'];
        if ($remote->stop_cam($camera)){
            $system_messages[] = 'Camera Stopped';
        } else {
            //stop failed
            $system_messages[] = 'Stopping Camera failed ' . $remote->get_error();
        }

    } elseif (!empty($_POST['create_camera'])){
        //create a camera
        $sql = "INSERT INTO config (name, value, camera) SELECT 'active', '0', MAX(camera) + 1 FROM config";
        $db->query($sql);
        $sql = 'SELECT MAX(camera) AS new_cam FROM config';
        $res = $db->query($sql);
        $row = $res->fetch_assoc();
        $camera_id = $row['new_cam'];
        $system_messages[] = "Created Camera $camera_id";
    } elseif (!empty($_POST['name'])){
        //update the camera
        foreach($_POST['name'] as $k => &$name){
            if (!empty($name)){
                //empty name is just ignored
                $camera = -1;
                if (isset($_POST['camera'][$k]) && strtoupper($_POST['camera'][$k]) != 'ALL'){
                    $camera = (int)$_POST['camera'][$k];
                }

                if (!empty($_POST['delete'][$k])){
                    //deleting this
                    $qname = $db->real_escape_string(trim($name));
                    $sql = "DELETE FROM config WHERE camera = $camera AND name = '$qname'";
                    $db->query($sql);
                } else {
                    //insert and update
                    $trimed_name = trim($name);
                    //we need to validate the name
                    if ($cfg->issettable($trimed_name)){
                        $qname = $db->real_escape_string($trimed_name);
                        $qval = $db->real_escape_string(trim($_POST['value'][$k]));
                        $sql = "REPLACE INTO config (name, value, camera) VALUES ('$qname', '$qval', $camera)";
                        $res = $db->query($sql);
                        if (!$res){
                            $smarty->assign('error_msg', 'Query Failed (' . $db->errno . ') ' . $db->error);
                            $smarty->display('error.tpl');
                            die();
                        }
                    }
                }
            }
        }
        $system_messages[] = 'Settings Updated, backend reload may be required';
    }
    //system config needs to be reloaded
    $cfg = new WebConfig($db);
}

//the end goal is this will always be a list of all possible settings
//each element will have:
// key -> The name of the element
// value -> The current value
// default -> the default value
// system -> The system value
// desc -> long description
// short_desc -> short description
// read_only -> true for read only values
// set -> the location it was set, 'default' (not set), 'system' (set as a system option), 'camera' (set for this specific camera)

$cfg_values = array();
$cfg_keys = $cfg->get_all_keys();
$unset_cfg_keys = array();//an array of keys not set at this level (may be set at the system level when viewing the camera)
$set_cfg_keys = array();
$camera_page = false;

if ($camera_id >= 0){
    $camera_config = new WebConfig($db, $camera_id);
    $camera_page = true;
    foreach ($cfg_keys as $k){
        $val = array();
        $val['key'] = $k;
        $val['value'] = $camera_config->get_value($k);
        $val['default'] = $cfg->get_default_value($k);
        $val['system'] = $cfg->get_value($k);
        $val['desc'] = $cfg->get_desc($k);
        $val['short_desc'] = $cfg->get_short_desc($k);
        $val['read_only'] = $cfg->get_priority($k) == SETTING_READ_ONLY;
        $val['required'] = $cfg->get_priority($k) == SETTING_REQUIRED_CAMERA;
        if ($camera_config->isset($k)){
            $val['set'] = 'camera';
            $set_cfg_keys[] = $k;
        } elseif ($cfg->isset($k)){
            $val['set'] = 'system';
            $set_cfg_keys[] = $k;//we have a value to display
            $unset_cfg_keys[] = $k;//but it can still be added
        } else {
            $val['set'] = 'default';
            if ($val['required']){//if the value is required it's pushed to the set list even when unset
                $set_cfg_keys[] = $k;
            } else {
                $unset_cfg_keys[] = $k;
            }
        }
        $cfg_values[$k] = $val;
    }

} else {
    //system settings
    foreach ($cfg_keys as $k){
        $val = array();
        $val['key'] = $k;
        $val['value'] = $cfg->get_value($k);
        $val['default'] = $cfg->get_default_value($k);
        $val['system'] = $cfg->get_value($k);
        $val['desc'] = $cfg->get_desc($k);
        $val['short_desc'] = $cfg->get_short_desc($k);
        $val['read_only'] = $cfg->get_priority($k) == SETTING_READ_ONLY;
        $val['required'] = $cfg->get_priority($k) == SETTING_REQUIRED_SYSTEM;
        if ($cfg->isset($k)){
            $val['set'] = 'system';
            $set_cfg_keys[] = $k;
        } else {
            $val['set'] = 'default';
            if ($val['required']){//if the value is required it's pushed to the set list even when unset
                $set_cfg_keys[] = $k;
            } else {
                $unset_cfg_keys[] = $k;
            }
        }
        $val['set'] = $cfg->isset($k) ? 'system' : 'default';
        $cfg_values[$k] = $val;
    }

}
$smarty->assign('timezones', DateTimeZone::listIdentifiers());
$smarty->assign('cfg_values', $cfg_values);
$smarty->assign('cfg_keys', $cfg_keys);
$smarty->assign('cfg_json', json_encode($cfg_values));
$smarty->assign('unset_keys', $unset_cfg_keys);
$smarty->assign('set_keys', $set_cfg_keys);
$smarty->assign('camera_setting', $camera_page);
$smarty->assign('camera_id', $camera_id);
//list all cameras
$sql = "SELECT name, value, camera FROM config WHERE name='active' OR name='display-name' ORDER BY camera ASC, name ASC";
$res = $db->query($sql);
$cameras = array();
while ($row = $res->fetch_assoc()){
    if ($row['camera'] < 0){
            continue;//ignore system settings
    }
    $name = $row['name'] == 'active' ? 'Camera ' . $row['camera'] : $row['value'];
    $cameras[$row['camera']]['name'] = $name;
    if ($row['name'] == 'active'){
        $cameras[$row['camera']]['active'] = $row['value'];
    }
}
$smarty->assign('camera_list', $cameras);

if (!empty($system_messages)){
    $smarty->assign('system_msg', $system_messages);
}

$smarty->display('settings.tpl');
