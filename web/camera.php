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
$start_ts = empty($_GET['start']) ? (new DateTime('midnight', $tz))->getTimeStamp() : (int)$_GET['start'];
$start_time = new Datetime("@" . $start_ts);
$packed_start = DateTime_to_dbtime($start_time);
$yesterday = $cur_time = new DateTime('now', $tz);;
$yesterday->sub(new DateInterval('P1D'));
$packed_endtime = -1;
if ($start_time->getTimestamp() < $yesterday->getTimestamp()){
    $endtime = new DateTime('@'.($start_time->getTimestamp() + 3600*24));
    $packed_endtime = Datetime_to_dbtime($endtime);
}

//get the camera settings
$camera_cfg = new WebConfig($db, $camera_id);
$smarty->assign('camera_name', $camera_cfg->get_value('display-name'));
$messages = array();


//check for POST
if ($_SERVER['REQUEST_METHOD'] == 'POST'){
    require_once('./inc/remote.php');
    if (!empty($_POST['from_d'])){
        $from_ts = (int)isset($_POST['from_d'])  ? $_POST['from_d'] : 0;
        $from_ts += (int)isset($_POST['from_h']) ? $_POST['from_h']*3600 : 0;
        $from_ts += (int)isset($_POST['from_m']) ? $_POST['from_m']*60 : 0;
        $from_ts += (int)isset($_POST['from_s']) ? $_POST['from_s'] : 0;

        $to_ts = (int)isset($_POST['to_d']) ? $_POST['to_d'] : 0;
        $to_ts += (int)isset($_POST['to_h'])? $_POST['to_h']*3600 : 0;
        $to_ts += (int)isset($_POST['to_m'])? $_POST['to_m']*60 : 0;
        $to_ts += (int)$_POST['to_s'];

        $start_ts = $from_ts;

        if ($to_ts <= $from_ts){
            $messages[] = 'To Time is before From Time';
        } else {
            $from_ts_packed = $from_ts * 1000;
            $to_ts_packed = $to_ts * 1000;

            $sql = "UPDATE videos SET locked = 1 WHERE camera = $camera_id AND (( endtime >= $from_ts_packed AND endtime <= $to_ts_packed ) ".
                 " OR (starttime >= $from_ts_packed AND starttime <= $to_ts_packed )) ";
            $db->query($sql);
            $messages[] = 'Locked ' . $db->affected_rows . ' segments';
        }
    } else if (!empty($_POST['unlock_segment'])){
        $from_ts = (int)isset($_POST['start_day_ts'])  ? $_POST['start_day_ts'] : 0;
        $from_ts += (int)isset($_POST['start_h']) ? $_POST['start_h']*3600 : 0;
        $from_ts += (int)isset($_POST['start_m']) ? $_POST['start_m']*60 : 0;
        $from_ts += (int)isset($_POST['start_s']) ? $_POST['start_s'] : 0;

        $to_ts = (int)isset($_POST['end_day_ts'])  ? $_POST['end_day_ts'] : 0;
        $to_ts += (int)isset($_POST['end_h']) ? $_POST['end_h']*3600 : 0;
        $to_ts += (int)isset($_POST['end_m']) ? $_POST['end_m']*60 : 0;
        $to_ts += (int)isset($_POST['end_s']) ? $_POST['end_s'] : 0;

        $start_ts = $from_ts;

        if ($to_ts <= $from_ts){
            $messages[] = 'To Time is before From Time';
        } else {
            $from_ts_packed = $from_ts * 1000;
            $to_ts_packed = $to_ts * 1000;

            $sql = "UPDATE videos SET locked = 0 WHERE camera = $camera_id AND (( endtime >= $from_ts_packed AND endtime <= $to_ts_packed ) ".
                 " OR (starttime >= $from_ts_packed AND starttime <= $to_ts_packed )) ";
            $db->query($sql);
            $messages[] = 'UnLocked ' . $db->affected_rows . ' segments';
        }

    } else if (!empty($_POST['export'])){
        $from_ts = (int)isset($_POST['start_day_ts'])  ? $_POST['start_day_ts'] : 0;
        $from_ts += (int)isset($_POST['start_h']) ? $_POST['start_h']*3600 : 0;
        $from_ts += (int)isset($_POST['start_m']) ? $_POST['start_m']*60 : 0;
        $from_ts += (int)isset($_POST['start_s']) ? $_POST['start_s'] : 0;

        $to_ts = (int)isset($_POST['end_day_ts'])  ? $_POST['end_day_ts'] : 0;
        $to_ts += (int)isset($_POST['end_h']) ? $_POST['end_h']*3600 : 0;
        $to_ts += (int)isset($_POST['end_m']) ? $_POST['end_m']*60 : 0;
        $to_ts += (int)isset($_POST['end_s']) ? $_POST['end_s'] : 0;

        $start_ts = $from_ts;

        if ($to_ts <= $from_ts){
            $messages[] = 'To Time is before From Time';
        } else {
            $from_ts_packed = $from_ts * 1000;
            $to_ts_packed = $to_ts * 1000;

            $sql = "INSERT INTO exports (camera, starttime, endtime) VALUES ($camera_id, $from_ts_packed, $to_ts_packed ) ";
            $db->query($sql);
            $messages[] = 'Export Job Started ';
        }

    } else if (!empty($_POST['delete_export'])){
        $export_id = empty($_POST['export_id']) ? 0 : (int)$_POST['export_id'];
        //we request the backend delete it since it manages files
        $remote = new Remote($db);
        if ($remote->rm_export($export_id)){
            $messages[] = 'Export Job Deleted ';
        } else {
            $messages[] = 'Could not delete export job, ' . $remote->get_error();
        }
    }
}

//the following queries are timezone sensitive, so lets tell the server our configured timezone
$tz_offset = $tz->getOffset(new DateTime('now'))/60;
$tz_min = $tz_offset%60;
if ($tz_min < 10){
    $tz_min = '0' . $tz_min;
}
$tz_offset = (int)$tz_offset/60 . ':' . $tz_min;
$sql = "SET time_zone = '$tz_offset'";
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

$sql = 'SELECT v.starttime, v.endtime, v1.locked AS lockend, v2.locked AS lockstart FROM videos AS v '.
 ' LEFT JOIN videos AS v1 ON v.endtime = v1.starttime AND v.id != v1.id AND v.camera = v1.camera '.
' LEFT JOIN videos AS v2 ON v.starttime = v2.endtime AND v.id != v2.id AND v.camera = v2.camera '.
 ' WHERE v.locked = 1 AND ((v1.locked IS NULL OR v1.locked = 0) OR (v2.locked IS NULL OR v2.locked = 0)) AND v.camera =  ' . $camera_id.
 ' ORDER by v.starttime ASC';
$res = $db->query($sql);
//echo $sql;
$lockstart = 0;
$lockend = 0;
$lock_data = array();
while ($row = $res->fetch_assoc()){
    if ($row['lockstart'] == 0 && $row['lockstart'] !== NULL){
        if ($lockend != 0){//last segment ended on a NULL, so push that
            $lock_data[] = generate_lock_info($lockstart, $lockend);
            $lockstart = 0;//reset the start
            $lockend = 0;//reset the end
        }
        $lockstart = $row['starttime'];
    } else if ($row['lockstart'] === NULL){
        if ($lockstart == 0){
            $lockstart = $row['starttime'];
        } // else we continue through the gap and don't change the start
    }

    if ($row['lockend'] == 0 && $row['lockend'] !== NULL){
        $lockend = $row['endtime'];
        $lock_data[] = generate_lock_info($lockstart, $lockend);
        $lockstart = 0;//reset the start
        $lockend = 0;//reset the end
    } else if ($row['lockend'] === NULL){
        $lockend = $row['endtime'];
    }

}
if ($lockend != 0 && $lockstart != 0){
    //we ended on a NULL lockend, so add that in now
    $lock_data[] = generate_lock_info($lockstart, $lockend);
}

function generate_lock_info($lockstart, $lockend){
    $lockinfo = array();
    $lockinfo['start_ts'] = $lockstart;
    $lockinfo['end_ts'] = $lockend;

    $start_date = dbtime_to_DateTime($lockstart);
    $end_date = dbtime_to_DateTime($lockend);
    //modify the end by rounding up one second
    $end_date->modify('+1 second');
    $start_interval = DateInterval::createFromDateString($start_date->format('H \h\o\u\r \+ i \m\i\n + s \s\e\c'));//the time with no date
    $end_interval = DateInterval::createFromDateString($end_date->format('H \h\o\u\r \+ i \m\i\n + s \s\e\c'));

    $lockinfo['start_txt'] = $start_date->format('D M jS H:i:s');
    $lockinfo['start_day_txt'] = $start_date->format('D M jS');
    $lockinfo['start_h'] = $start_date->format('H');
    $lockinfo['start_m'] = $start_date->format('i');
    $lockinfo['start_s'] = $start_date->format('s');

    $start_date->sub($start_interval);
    $lockinfo['start_day_ts'] = $start_date->getTimestamp();

    $lockinfo['end_txt'] = $end_date->format('D M jS H:i:s');
    $lockinfo['end_day_txt'] = $end_date->format('D M jS');
    $lockinfo['end_h'] = $end_date->format('H');
    $lockinfo['end_m'] = $end_date->format('i');
    $lockinfo['end_s'] = $end_date->format('s');

    $end_date->sub($end_interval);
    $lockinfo['end_day_ts'] = $end_date->getTimestamp();
    return $lockinfo;
}
$smarty->assign('locked_videos', $lock_data);

//get the list of exports
$exports = array();
$sql = "SELECT id, progress, path, starttime, endtime FROM exports WHERE camera = $camera_id ORDER BY starttime ASC";
$res = $db->query($sql);
if ($res){
    while ($row = $res->fetch_assoc()){
        $exp_info = array();
        $exp_info['id'] = $row['id'];
        $exp_info['url'] = 'vids/' . $row['path']  . $row['id'] . '.mp4';
        $exp_info['progress'] = $row['progress'];
        $start_date = dbtime_to_DateTime($row['starttime']);
        $end_date = dbtime_to_DateTime($row['endtime']);
        $exp_info['start_txt'] = $start_date->format('D M jS H:i:s');
        $exp_info['end_txt'] = $end_date->format('D M jS H:i:s');
        $exports[] = $exp_info;
    }
}
$smarty->assign('exports', $exports);
$video_info = array('url' => 'stream.php?id=' . (int)$_GET['id'] . '&start='. $start_ts,
                    'camera' => (int)$_GET['id'],
                    'start_ts' => $start_ts
);

$smarty->assign('video_info', $video_info);


//get the events
$events = array();
$sql = 'SELECT e.id, e.source, e.starttime, e.x0, e.y0, e.x1, e.y1, e.score, e.img, i.path, i.prefix, i.extension '
     .' FROM events AS e LEFT JOIN images AS i ON e.img = i.id '
     .' WHERE e.starttime >= ' . $packed_start;
if ($packed_endtime != -1){
    $sql .= ' AND e.starttime < '.  $packed_endtime;
}
$sql .= ' ORDER BY e.starttime DESC LIMIT 100';

$res = $db->query($sql);
if ($res){
    while ($row = $res->fetch_assoc()){
        $ev_info = array();
        $ev_info['id'] = $row['id'];
        if (!empty($row['path'])){
            $ev_info['img'] = 'vids/' . $row['path']  . $row['prefix'] . $row['img'] . $row['extension'];
        }
        $ev_info['score'] = $row['score'];
        $ev_info['source'] = $row['source'];
        $start_date = dbtime_to_DateTime($row['starttime']);
        $ev_info['start_txt'] = $start_date->format('D M jS H:i:s');
        $ev_info['start_ts'] = $start_date->getTimestamp();
        $ev_info['pos'][0]['x'] = $row['x0'];
        $ev_info['pos'][0]['y'] = $row['y0'];
        $ev_info['pos'][1]['x'] = $row['x1'];
        $ev_info['pos'][1]['y'] = $row['y1'];
        $events[] = $ev_info;
    }
}
$smarty->assign('events', $events);

$smarty->assign('system_msg', $messages);

$smarty->display('camera.tpl');
