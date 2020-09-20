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
if ($_SERVER['REQUEST_METHOD'] == 'POST'){
    //update the DB...
    
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
                $qname = $db->real_escape_string(trim($name));
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
$camera_id = -1;
if (isset($_GET['camera']) && $_GET['camera'] >= 0){
    $camera_config = WebConfig($db, (int)$_GET['camera']);
    $camera_page = true;
    $camera_id = (int)$_GET['camera'];
    foreach ($cfg_keys as $k){
        $val = array();
        $val['key'] = $k;
        $val['value'] = $camera_config->get_value($k);
        $val['default'] = $cfg->get_default_value($k);
        $val['system'] = $cfg->get_value($k);
        $val['desc'] = $cfg->get_desc($k);
        $val['short_desc'] = $cfg->get_short_desc($k);
        $val['read_only'] = $cfg->get_priority($k) == READ_ONLY;
        if ($camera_config->isset($k)){
            if ($cfg->isset($k)){
                $val['set'] = 'system';
                $unset_cfg_keys[] = $k;
            } else {
                $val['set'] = 'camera';
                $set_cfg_keys[] = $k;
            }
        } else {
            $val['set'] = 'default';
            $unset_cfg_keys[] = $k;
        }
        $val['set'] = $cfg->isset($k) ? 'system' : 'default';
        $cfg_values[$key] = $val;
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
        if ($cfg->isset($k)){
            $val['set'] = 'system';
            $set_cfg_keys[] = $k;
        } else {
            $val['set'] = 'default';
            $unset_cfg_keys[] = $k;
        }
        $val['set'] = $cfg->isset($k) ? 'system' : 'default';
        $cfg_values[$k] = $val;
    }

}
$smarty->assign('timezones', DateTimeZone::listIdentifiers());
$smarty->assign('cfg_values', $cfg_values);
$smarty->assign('cfg_keys', $cfg_keys);
$smarty->assign('unset_keys', $unset_cfg_keys);
$smarty->assign('set_keys', $set_cfg_keys);
$smarty->assign('camera_setting', $camera_page);
$smarty->assign('camera_id', $camera_id);
$smarty->display('settings.tpl');
