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

ob_start("ob_gzhandler");
$suppress_smarty = true;
require_once('./inc/main.php');


if (!isset($_GET['id']) || ((int)$_GET['id']) < 0){
    die('No camera given');
}
$camera = (int)$_GET['id'];

$cur_time = new DateTime('now', $tz);
header('Access-Control-Allow-Origin: *');
header('Access-Control-Expose-Headers: Content-Length');
header('Access-Control-Allow-Headers: Range');

if (!empty($_GET['start']) && ((int)$_GET['start']) != 0){
    //start time was given
    $start_time = new Datetime("@" . (int)$_GET['start']);
} else {
    $cur_time->setTime(0, 0);//midnight
    header("Location: info.php?id=$camera&start=" . $cur_time->getTimestamp());
    die();
}

header('Content-type: application/json');

$packed_start = DateTime_to_dbtime($start_time);

$yesterday = clone $cur_time;
$yesterday->sub(new DateInterval('P1D'));

//compute if this is limited to 24 hours
if ($start_time->getTimestamp() < $yesterday->getTimestamp()){
    $endtime = new DateTime('@'.($start_time->getTimestamp() + 3600*24));
    $packed_endtime = Datetime_to_dbtime($endtime);
}

$json = array(
    'camera' => (int)$_GET['id'],
    'starttime' => $start_time->getTimestamp()//FIXME: should be actual start time
);


//query the gaps in the video
$sql = 'SELECT v.starttime, v.endtime, v1.starttime AS nostart, v2.starttime AS noend FROM '.
 ' videos AS v LEFT JOIN videos AS v1 ON v.endtime = v1.starttime AND v.id != v1.id AND v.camera = v1.camera '.
 ' LEFT JOIN videos AS v2 ON v.starttime = v2.endtime AND v.id != v2.id  AND v.camera = v2.camera '.
 ' WHERE v.camera = ' . ((int)$_GET['id']).' AND v.endtime IS NOT NULL AND (v1.starttime IS NULL OR v2.endtime IS NULL) AND v.starttime > ' . $packed_start;
if (isset($packed_endtime)){
    $sql .= ' AND v.starttime < '. $packed_endtime;
}
$sql .= ' ORDER BY v.starttime ASC';
$res = $db->query($sql);


/*
 * Generate Json Gap Data
 *
 *  - len - Length of gap in seconds
 *  - start_ts - time since $start_time (last midnight usually) that the gap starts
 *  - actual_start_ts - Time on the actual received video that the gap starts
 *
 */
if ($res){
    $gap = array();
    $end_ts = $packed_start;
    $total_gaps = 0;
    $last_endtime = 0;
    while ($row = $res->fetch_assoc()){
        //mark the end of the video
        if ($row['nostart'] === NULL){
            $end_ts = $row['endtime'];
            continue;
        }

        //found the start of video, compute the gap and metadata
        if ($row['noend'] === NULL){
            $start_ts = dbtime_to_Datetime($end_ts);//$end_ts is the end of the last file, which would start the gap (so it's the start time of the gap)

            //compute the gap
            $gap_stat['len'] = ($row['starttime'] - $end_ts)/1000.0;//$row['starttime'] is the start of the next file, so it's the end of the gap
            $ts = $start_ts->diff($start_time);
            $gap_stat['start_ts'] = DateInterval_to_ts($ts);
            $gap_stat['actual_start_ts'] = $gap_stat['start_ts'] - $total_gaps;
            $total_gaps += $gap_stat['len'];//to track how our timestamps differ
            $gap[] = $gap_stat;
        }
    }
    $json['gaps'] = $gap;
} else {
    //query failed
    die($sql);
}

echo json_encode($json);

ob_end_flush();
?>
