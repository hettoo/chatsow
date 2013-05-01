<?php

import_lib('Table');

$shared['head'] = '';

$table = new Table();
$table->addColumn(array('title' => 'Map', 'size' => 'large'));
$table->addColumn(array('title' => 'Record holder', 'column' => 'name_raw', 'size' => 'large'));
$table->addColumn(array('title' => 'Record', 'align' => 'right', 'size' => 'small'));
$table->addColumn(array('title' => 'Date', 'align' => 'right', 'size' => 'medium'));

$result = $db->query("SELECT P.`id`, M.`name`, `record`, P.`name` AS `record_holder`, UNIX_TIMESTAMP(`timestamp`) AS `timestamp` FROM `map` M, `player` P WHERE P.`id`=M.`player` ORDER BY `timestamp` DESC LIMIT 8") or die($db->error);
while ($row = $result->fetch_array()) {
    $table->addField(format_map($row['name']));
    $table->addField(format_player($row['record_holder'], $row['id'], -1));
    $table->addField(format_time($row['record'], $row['name']));
    $table->addField(format_date_relative($row['timestamp']));
}

$map_count = 0;
$player_count = 0;
$result = $db->query("SELECT COUNT(*) AS `maps`, COUNT(DISTINCT `player`) AS `players` FROM `map`") or die($db->error);
if ($row = $result->fetch_array()) {
    $map_count = $row['maps'];
    $player_count = $row['players'];
}

$shared['map_count'] = $map_count;
$shared['player_count'] = $player_count;
$shared['table'] = $table;

?>
