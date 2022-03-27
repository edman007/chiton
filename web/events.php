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
 *   Copyright 2020-2021 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

require_once('./inc/main.php');

if (!isset($_GET['id']) || ((int)$_GET['id']) < 0){
    die('No camera given');
}
$camera_id = (int)$_GET['id'];
$ev_start_id = empty($_GET['ev_id']) ? -1 : (int)$_GET['ev_id'];
$ev_start = empty($_GET['ev_start']) ? -1 : (int)$_GET['ev_start'] * 1000;
$cam_start = empty($_GET['start']) ? -1 : (int)$_GET['start'] * 1000;

$camera_cfg = new WebConfig($db, $camera_id);
$events = new Events($db, $camera_cfg, $camera_id);
$smarty->assign('events', $events->get_events($cam_start, $ev_start, $ev_start_id));
$smarty->assign('raw_events', true);
$smarty->assign('camera_id', $camera_id);
$smarty->display('events.tpl');
