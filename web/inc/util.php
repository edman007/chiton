<?php

function dbtime_to_DateTime($timestamp){
    global $cfg, $tz;
    $datetime = new DateTime();
    $datetime->setTimeStamp($timestamp/1000);
    if (!empty($tz)){
        $datetime->setTimeZone($tz);
    }

    return $datetime;
}

function DateTime_to_dbtime($datetime){
    return $datetime->getTimeStamp()*1000;
}

?>