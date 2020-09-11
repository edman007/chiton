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
$smarty->assign('pagename', 'Licenses');

//guess the path for the license directory
$license_path = '../licenses/';
if (!file_exists($license_path . 'COPYING')){
    //try the alternate path
    $license_path = '../';
    if (!file_exists($license_path . 'COPYING')){
        //check your config!
        $smarty->assign('no_license', true);
        $smarty->display('license.tpl');
        die();
    }
}

$license_txt = array();
$license_txt['chiton'] = file_get_contents($license_path . 'COPYING');
$license_files = glob($license_path . 'LICENSE.*');
foreach ($license_files as $file){
    $name = explode('.', basename($file), 2);
    $license_txt['third_party'][] = array('name' => $name[1],
                                          'text' => file_get_contents($file));
}
$smarty->assign('license', $license_txt);


$smarty->display('license.tpl');


?>
