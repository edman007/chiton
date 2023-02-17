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
 *   Copyright 2023 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

require_once('./inc/main.php');

$smarty->assign('title', 'System Status');
$camera_id = -1;
$system_messages = array();
if (isset($_GET['camera'])){
    $camera_id = (int)$_GET['camera'];
}
$smarty->assign('camera_id', $camera_id);


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

if (isset($cameras[$camera_id]['name'])){
    $smarty->assign('display_name', $cameras[$camera_id]['name']);
}

$smarty->assign('camera_list', $cameras);
$remote = new Remote($db);
$log = $remote->log_cam($camera_id);
if ($log !== false){
    $smarty->assign('log_msg', $log);
} else {
    $error_msg = $remote->get_error();
    if (!empty($error_msg)){
        $system_messages[] = $error_msg;
    }
}

$status = $remote->list_status();
if ($status !== false){
    $smarty->assign('cam_status', $status);
} else {
    $error_msg = $remote->get_error();
    if (!empty($error_msg)){
        $system_messages[] = $error_msg;
    }
}


if (!empty($system_messages)){
    $smarty->assign('system_msg', $system_messages);
}

$smarty->display('stats.tpl');


?>
