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
$camera_id = empty($_GET['id']) ? 0 : (int)$_GET['id'];
$smarty->assign('camera_id', $camera_id);


//get the camera settings
$camera_cfg = new WebConfig($db, $camera_id);
$smarty->assign('camera_name', $camera_cfg->get_value('display-name'));

//the following queries are timezone sensitive, so lets tell the server our configured timezone
$tz_offset = $tz->getOffset(new DateTime('now'))/60;
$tz_min = $tz_offset%60;
if ($tz_min < 10){
    $tz_min = '0' . $tz_min;
}
$tz_offset = (int)$tz_offset/60 . ':' . $tz_min;
$sql = "SET timezone = '$tz_offset'";
$db->query($sql);



//get the oldest possible date
$sql = "SELECT FROM_UNIXTIME(starttime/1000, '%Y-%m-%d') as day, COUNT(id) FROM videos WHERE  camera = ".($camera_id).' GROUP BY day ORDER BY starttime DESC';
$res = $db->query($sql);
if ($res){
    $available_days = array();
    $found_selected = false;
    while ($row = $res->fetch_assoc()){
        $avail_info = array();
        $avail_date = new DateTime($row['day'], $tz);
        $avail_info['long'] = $avail_date->format('D M jS');
        $avail_info['timestamp'] = $avail_date->getTimestamp();
        $avail_info['stream'] = 'stream.php?id='.((int)$_GET['id']). '&start=' . $avail_date->getTimestamp();
        if (!$found_selected && empty($_GET['start'])){
            $avail_info['selected'] = 1;
            $found_selected = true;
        } else if (!$found_selected && !empty($_GET['start']) && $_GET['start'] == $avail_info['timestamp']){
            $avail_info['selected'] = 1;
            $found_selected = true;
        } else {
            $avail_info['selected'] = 0;
        }
        $available_days[] = $avail_info;
    }
    $smarty->assign('avail_days', $available_days);
}

$start_ts = empty($_GET['start']) ? (new DateTime('midnight', $tz))->getTimeStamp() : (int)$_GET['start'];
$video_info = array('url' => 'stream.php?id=' . (int)$_GET['id'] . '&start='. $start_ts,
    'camera' => (int)$_GET['id'],
    'start_ts' => $start_ts
);
$smarty->assign('video_info', $video_info);

$smarty->display('camera.tpl');

?>
