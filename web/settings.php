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

$smarty->assign('title', 'Settings');
if ($_SERVER['REQUEST_METHOD'] == 'POST'){
    //update the DB...
    
    foreach($_POST['name'] as $k => &$name){
        if (!empty($name)){
            //empty name is just ignored
            $camera = -1;
            if (isset($_POST['camera'][$k]) && strtoupper($_POST['camera'][$k]) != 'ALL'){
                $camera = (int)$_POST['camera'][$k];
            }

            if (!empty($_POST['delete'][$k]) || empty($_POST['value'][$k])){
                //deleting this
                $qname = $db->real_escape_string(trim($name));
                $sql = "DELETE FROM config WHERE camera = $camera AND name = '$qname'";
                $db->query($sql);
            } else {
                //insert and update
                $qname = $db->real_escape_string(trim($name));
                $qval = $db->real_escape_string(trim($_POST['value'][$k]));
                $sql = "REPLACE INTO config (name, value, camera) VALUES ('$qname', '$qval', $camera)";
                $res = $db->query($sql);
                if (!$res){
                    $smarty->assign('error_msg', 'Query Failed (' . $db->errno . ') ' . $db->error);
                    $smarty->display('error.tpl');
                    die();
                }
            }
        }
    }
}

$sql = 'SELECT name, value, camera FROM config ORDER by camera, name';
$res = $db->query($sql);
if ($res){
    $smarty->assign('cfg_options', $res->fetch_all(MYSQLI_ASSOC));
    
} else {
    $smarty->assign('error_msg', 'Query Failed (' . $db->errno . ') ' . $db->error);
    $smarty->display('error.tpl');
    die();

}

$smarty->display('settings.tpl');


?>
