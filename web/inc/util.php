<?php

function dbtime_to_DateTime($timestamp){
    global $cfg, $tz;
    $datetime = new DateTime();
    $datetime->setTimeStamp($timestamp/1000);
    $datetime->modify('+' . (($timestamp % 1000) * 1000) . ' microseconds');
    if (!empty($tz)){
        $datetime->setTimeZone($tz);
    }

    return $datetime;
}

function DateTime_to_dbtime($datetime){
    return $datetime->getTimeStamp()*1000;
}

function DateInterval_to_ts($dateInterval){
    return 0.0 + $dateInterval->h*3600 + $dateInterval->i*60 + $dateInterval->s + $dateInterval->f;
}
?>
