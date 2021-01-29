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
} elseif (!empty($_GET['live'])) {
    //preview, link to a quickload
    $cur_time->sub(new DateInterval("PT1M"));
    header("Location: stream.php?id=$camera&start=" . $cur_time->getTimestamp());
    die();
} else {
    $cur_time->setTime(0, 0);//midnight
    header("Location: stream.php?id=$camera&start=" . $cur_time->getTimestamp());
    die();
}

header('Content-type: application/vnd.apple.mpegurl');

$camera_cfg = new WebConfig($db, $camera);
$packed_start = DateTime_to_dbtime($start_time);

$yesterday = clone $cur_time;
$yesterday->sub(new DateInterval('P1D'));
//$yesterday->setTime(0,0);
//query the most recent video from the DB
$sql = 'SELECT id, path, starttime, endtime, extension FROM videos WHERE camera = ' . ((int)$_GET['id']).' AND endtime IS NOT NULL AND starttime > ' . $packed_start;
if ($start_time->getTimestamp() < $yesterday->getTimestamp()){
    $endtime = new DateTime('@'.($start_time->getTimestamp() + 3600*24));
    $packed_endtime = Datetime_to_dbtime($endtime);
    $sql .= ' AND starttime < '. $packed_endtime;
}
$sql .= ' ORDER BY starttime ASC';
$res = $db->query($sql);
if ($res){
    //add the header
    echo "#EXTM3U\n";
    echo "#EXT-X-VERSION:3\n";
    echo '#EXT-X-PROGRAM-DATE-TIME:' . $start_time->format('c') . "\n";
    echo "#EXT-X-PLAYLIST-TYPE:EVENT\n";
    echo "#EXT-X-TARGETDURATION:" . (2*$camera_cfg->get_value('seconds-per-file')). "\n";
    
    //master playlist requirements
    //EXT-X-STREAM-INF - CODECS and RESOLUTION
    $last_endtime = 0;
    while ($row = $res->fetch_assoc()){
        if ($last_endtime != 0 && $row['starttime'] != $last_endtime){
            echo "#EXT-X-DISCONTINUITY\n";
        }
        $last_endtime = $row['endtime'];
        $len = ($row['endtime'] - $row['starttime'])/1000;
        if ($len == 0){//we have seen some with a len of zero...is this an ok workaround?
            $len = 0.001;
        }
        $url = 'vids/' . $row['path'] . $row['id'] . $row['extension'];
        $name = "Camera $camera: " . dbtime_to_DateTime($row['starttime'])->format('r');
        echo "#EXTINF:$len,$name\n";
        echo $url . "\n";
    }
    
    if (!empty($endtime)){ 
        //this is a complete playlist, add the end of the file
        echo "#EXT-X-ENDLIST\n";
    }

} else {
    //query failed
    die($sql);
}


header('Accept-Ranges: bytes');
function range_explode($str){
    global $full_len;
    $ret = explode('-', $str, 2);
    $ret[0] = (int)trim($ret[0]);
    $ret[1] = trim($ret[1]);
    if ($ret[1] == ''){
        //then it is the max
        $ret[1] = $full_len;
    } else {
        $ret[1] = (int)$ret[1];
    }
    return $ret;
}


if (isset($_SERVER['HTTP_RANGE'])){
    //cut the output...
    $full_page = ob_get_contents();
    $full_len = strlen($full_page);
    ob_clean();
    $range = substr($_SERVER['HTTP_RANGE'], strpos($_SERVER['HTTP_RANGE'], '=') + 1);
    $range = explode(',', $range);
    $range = array_map('range_explode', $range);
    //technically we could do a multipart, but it's not going to be needed
    header('HTTP/1.1 206 Partial Content');
    if ($range[0][1] <= $range[0][0] || $range[0][1] > $full_len){
        header('HTTP/1.1 416 Requested Range Not Satisfiable');
        header("Content-Range: bytes {$range[0][0]}-{$range[0][1]}/$full_len");
        ob_end_clean();
        die();
    }
    $len = $range[0][1] - $range[0][0] + 1;
    header("Content-Range: bytes {$range[0][0]}-{$range[0][1]}/$full_len");
    header("Content-Length: $len");
    echo substr($full_page, $range[0][0], $len);


} else {
    header("Content-length: ".ob_get_length());
}
ob_end_flush();
?>
