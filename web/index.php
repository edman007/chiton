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
$smarty->assign('pagename', 'Home');

//query the most recent video from the DB
$sql = "SELECT camera, name, value FROM config WHERE name='active' OR name='display-name' ORDER BY camera ASC, name ASC";
$res = $db->query($sql);
$video_info = array();
$starttime = new Datetime('-45 seconds', $tz);
if ($res){
    while ($row = $res->fetch_assoc()){
        if ($row['name'] == 'display-name'){
            $video_info[array_key_last($video_info)]['name'] = $row['value'];
            continue;
        }
        $info['url'] = 'stream.php?start=' . $starttime->getTimeStamp() . '&id=' . $row['camera'];
        $info['start_ts'] = $starttime->getTimeStamp();
        $info['camera'] = $row['camera'];
        $info['active'] = $row['value'];
        $video_info[] = $info;
    }
    $smarty->assign('video_info', $video_info);
}


$smarty->display('index.tpl');

?>
