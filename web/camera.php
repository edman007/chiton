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
$smarty->assign('pagename', 'Camera View');

if (!isset($_GET['id'])){
    $smarty->assign('error_msg', 'No Camera Supplied');
    $smarty->display('error.tpl');
    die();
}
//query the most recent video from the DB
$sql = 'SELECT id, path, starttime, endtime FROM videos WHERE camera = ' . ((int)$_GET['id']).' ORDER BY starttime DESC';
$res = $db->query($sql);
$video_info = array();
if ($res){
    while ($row = $res->fetch_assoc()){
        $info['url'] = 'vids/' . $row['path'] . $row['id'] . '.mp4';
        $info['starttime'] = dbtime_to_DateTime($row['starttime'], $cfg)->format('r');
        $info['id'] = $row['id'];
        $video_info[] = $info;
    }
    $smarty->assign('video_info', $video_info);
}


$smarty->display('camera.tpl');

?>
