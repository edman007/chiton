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

//Find the config file
$cfg_path = getenv('CFG_DIR');
if ($cfg_path === FALSE){
    $cfg_path = './inc/config.php';
} else {
    $cfg_path .= '/config.php';
}

require_once($cfg_path);

require_once('./inc/external/smarty/Smarty.class.php');

require_once('./inc/configdb.php');

require_once('./inc/util.php');

require_once('./inc/web_config.php');

if (empty($suppress_smarty)){
    $smarty = new Smarty;
    //$smarty->force_compile = true;
    //$smarty->debugging = true;
    $smarty->caching = false;//caching never makes sense for use, it's updated live all the time and outside of PHP
    //$smarty->cache_lifetime = 120;
    $smarty->setTemplateDir('./inc/tpl')
        ->setCompileDir(SMARTY_COMPILE_DIR)
        ->setCacheDir(SMARTY_CACHE_DIR)
        ->setConfigDir('./inc/smarty_cfg');
}
$db = new mysqli(DB_HOST, DB_USER, DB_PASS, DB_DB);

//we need to do something to talk to the server soon....

if ($db->connect_error){
    if (empty($suppress_smarty)){
        $smarty->assign('error_msg', 'Database Connect Error (' . $db->connect_errno . ') '
            . $db->connect_error);
        $smarty->display('error.tpl');
    } else {
        echo 'Database Connect Error (' . $db->connect_errno . ') ' . $db->connect_error;
    }
        
    die();
}

$cfg = new WebConfig($db);

if ($cfg->get_value('timezone') != ''){
    $tz = new DateTimeZone($cfg->get_value('timezone'));
} else {
    $tz = new DateTimeZone(date_default_timezone_get());
}

?>
