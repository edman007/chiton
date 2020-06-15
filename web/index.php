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
$sql = 'SELECT id, camera, path, starttime, endtime FROM videos GROUP BY camera ORDER BY camera ASC, starttime DESC';
$res = $db->query($sql);
$video_info = array();
if ($res){
    while ($row = $res->fetch_assoc()){
        $info['url'] = 'stream.php?live=1&id=' . $row['camera'];
        $info['starttime'] = dbtime_to_DateTime($row['starttime'])->format('r');
        $info['camera'] = $row['camera'];
        $video_info[] = $info;
    }
    $smarty->assign('video_info', $video_info);
}


$smarty->display('index.tpl');

?>
