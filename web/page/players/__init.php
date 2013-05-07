<?php

import_lib('Pager');
import_lib('Table');
import_lib('Search');

$id = (int)$hierarchy[1];

$table = new Table(2);
$table->addColumn(array('name' => 'name', 'title' => 'Map', 'size' => 'large'));
$table->addColumn(array('name' => 'record', 'title' => 'Record', 'align' => 'right', 'size' => 'small'));
$table->addColumn(array('name' => 'timestamp', 'title' => 'Date', 'align' => 'right', 'size' => 'medium'));

$table->processOrder('name');

$search = new Search(4);
$like = $search->get();

$pager = new Pager(3, $s['max_rows'], "`name`, `record`, UNIX_TIMESTAMP(`timestamp`) AS `timestamp` FROM `map` WHERE `player`=$id AND `name` LIKE '%$like%'" . $table->getOrder());

$search->redirect($pager);

$maps = '';
$pager->query();
$rows = $pager->getRows();
foreach ($rows as $row) {
    $table->addField(format_map($row['name']));
    $table->addField(format_time($row['record'], $row['name']));
    $table->addField(format_date($row['timestamp']));
}

$result = $s['db']->query("SELECT `id`, `name` FROM `player` WHERE `id`=" . $id) or die($s['db']->error);
while ($row = $result->fetch_array()) {
    $formatted = format_player($row['name'], $row['id']);
    $s['head'] = 'Player ' . $formatted;
    $s['description'] = 'Record demos for ' . strip_tags($formatted) . '.';
    $s['player'] = $row;
}
$table->setPager($pager);
$table->setSearch($search);

$s['table'] = $table;

?>
