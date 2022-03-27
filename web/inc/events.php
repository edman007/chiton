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
class Events {
    private $cfg;
    private $db;
    private $id;

    function __construct($db, $cfg, $camera_id){
        $this->db = $db;
        $this->cfg = $cfg;
        $this->id = $camera_id;
    }

    //return events between $packed_start and $packed_endtime, if $ev_start_id is set, also only start returning items after that id
    function get_events($packed_start, $packed_endtime = -1, $ev_start_id = -1){
        $sql = 'SELECT e.id, e.source, e.starttime, e.x0, e.y0, e.x1, e.y1, e.score, e.img, i.path, i.prefix, i.extension '
             .' FROM events AS e LEFT JOIN images AS i ON e.img = i.id '
             .' WHERE e.starttime >= ' . $packed_start . ' AND e.camera = ' . $this->id;
        if ($packed_endtime != -1){
            $sql .= ' AND e.starttime <= '.  ($packed_endtime + 1000);
        }
        $sql .= ' ORDER BY e.starttime DESC LIMIT 100';

        $res = $this->db->query($sql);
        $events = array();
        if ($res){
            while ($row = $res->fetch_assoc()){
                if ($ev_start_id != -1 && $row['starttime'] > ($packed_endtime)){
                    if ($row['id'] == $ev_start_id){
                        $ev_start_id = -1;
                    }
                    continue;
                }
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
        return $events;
    }
}
