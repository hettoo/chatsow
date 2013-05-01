<?php

import_lib('Pager');
import_lib('Table');

$id = (int)$hierarchy[1];

$table = new Table(2);
$table->addColumn(array('name' => 'name', 'title' => 'Map'));
$table->addColumn(array('name' => 'record', 'title' => 'Record', 'align' => 'right'));
$table->addColumn(array('name' => 'timestamp', 'title' => 'Date', 'align' => 'right'));

$table->processOrder('name');

$like = search_get(4);

$pager = new Pager(3, $shared['max_rows'], "`name`, `record`, UNIX_TIMESTAMP(`timestamp`) AS `timestamp` FROM `map` WHERE `player`=$id AND `name` LIKE '%$like%'" . $table->getOrder());

search_redirect(4, $pager, $_POST['name']);

$maps = '';
$pager->query();
$rows = $pager->getRows();
foreach ($rows as $row) {
    $table->addField(format_map($row['name']));
    $table->addField(format_time($row['record'], $row['name']));
    $table->addField(format_date($row['timestamp']));
}

$result = $db->query("SELECT `id`, `name` FROM `player` WHERE `id`=" . $id) or die($db->error);
while ($row = $result->fetch_array()) {
    $shared['head'] = 'Player ' . format_player($row['name'], $row['id']);
    $shared['player'] = $row;
}

$shared['like'] = $like;
$shared['table'] = $table;
$shared['pager'] = $pager;

?>
