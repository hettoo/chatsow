<?php

if (isset($s['rs_name']))
    $s['rs'] = new mysqli($s['rs_host'], $s['rs_user'], $s['rs_password'], $s['rs_database']) or die("Unable to connect to the database.");

function rs_get_record($map) {
    global $s;
    $result = $s['rs']->query("SELECT `time` FROM `player_map` P, `map` M WHERE P.`map_id` = M.`id` AND `name`='$map' AND `time` IS NOT NULL ORDER BY `time` ASC LIMIT 1") or die($s['rs']->error);
    if ($row = $result->fetch_array())
        return $row['time'];
    return null;
}

function rs_diff($map, $time) {
    $rec = rs_get_record($map);
    if ($rec == null)
        return 'N/A';
    return format_time($rec - $time);
}

?>
