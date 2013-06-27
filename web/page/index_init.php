<?php

import_lib('Table');
import_lib('racesow');

$s['head'] = '';

$table = new Table();
$table->addColumn(array('title' => 'Map', 'size' => 'large'));
$table->addColumn(array('title' => 'Record holder', 'column' => 'name_raw', 'size' => 'large'));
$table->addColumn(array('title' => 'Record', 'align' => 'right', 'size' => 'small'));
if (isset($s['rs']))
    $table->addColumn(array('title' => $s['rs_name'] . ' diff', 'align' => 'right', 'size' => 'small'));
$table->addColumn(array('title' => 'Date', 'align' => 'right', 'size' => 'medium'));

$result = $s['db']->query("SELECT P.`id`, M.`name`, `record`, P.`name` AS `record_holder`, UNIX_TIMESTAMP(`timestamp`) AS `timestamp` FROM `map` M, `player` P WHERE P.`id`=M.`player` ORDER BY `timestamp` DESC LIMIT 8") or die($s['db']->error);
while ($row = $result->fetch_array()) {
    $table->addField(format_map($row['name']));
    $table->addField(format_player($row['record_holder'], $row['id'], -1));
    $table->addField(format_time($row['record'], $row['name']));
    if (isset($s['rs']))
        $table->addField(rs_diff($row['name'], $row['record']));
    $table->addField(format_date_relative($row['timestamp']));
}

$map_count = 0;
$player_count = 0;
$time = 0;
$result = $s['db']->query("SELECT COUNT(*) AS `maps`, COUNT(DISTINCT `player`) AS `players`, SUM(`record`) AS `time` FROM `map`") or die($s['db']->error);
if ($row = $result->fetch_array()) {
    $map_count = $row['maps'];
    $player_count = $row['players'];
    $time = $row['time'];
}

$s['map_count'] = $map_count;
$s['player_count'] = $player_count;
$s['time'] = $time;
$s['table'] = $table;

?>
