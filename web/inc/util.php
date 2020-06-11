<?php

function dbtime_to_DateTime($timestamp, $cfg){
    $datetime = new DateTime();
    $datetime->setTimeStamp($timestamp/1000);
    if ($cfg->get_value('timezone') != ''){
        $tz = new DateTimeZone($cfg->get_value('timezone'));
        $datetime->setTimeZone($tz);
    }

    return $datetime;
}

function DateTime_to_dbtime($datetime){
    return $datetime->getTimeStamp()*1000;
}

?>