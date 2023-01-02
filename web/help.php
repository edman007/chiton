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
 *   Copyright 2023 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

require_once('./inc/main.php');
$smarty->assign('pagename', 'Help');

$cfg_txt = array();
$keys = $cfg->get_all_keys();
foreach ($keys as $k){
    $cfg_txt[$k]['key'] = $k;
    $cfg_txt[$k]['default'] = $cfg->get_default_value($k);
    $cfg_txt[$k]['name'] = $cfg->get_short_desc($k);
    $cfg_txt[$k]['short'] = $cfg->get_desc($k);
    $cfg_txt[$k]['priority'] = $cfg->get_priority($k);
}

function sort_cfg($a, $b){
    if ($a['priority'] == $b['priority']){
        return strcmp($a['name'], $b['name']);
    } else {
        return $a['priority'] > $b['priority'];
    }

}
usort($cfg_txt, 'sort_cfg');

$last_priority = -1;
foreach ($cfg_txt as $k => &$v){
    if ($v['priority'] != $last_priority){
        $last_priority = $v['priority'];
        switch ($v['priority']){
        case SETTING_READ_ONLY:
            $v['priority_name'] = 'Read Only';
            break;
        case SETTING_REQUIRED_SYSTEM:
            $v['priority_name'] = 'Required System';
            break;
        case SETTING_OPTIONAL_SYSTEM:
            $v['priority_name'] = 'Optional System';
            break;
        case SETTING_REQUIRED_CAMERA:
            $v['priority_name'] = 'Required Camera';
            break;
        case SETTING_OPTIONAL_CAMERA:
            $v['priority_name'] = 'Optional Camera';
            break;
        default:
            $v['priority_name'] = 'Unknown (please report this as a bug)';
        }

    }
}

$smarty->assign('cfg_help', $cfg_txt);


$smarty->display('help.tpl');


?>
