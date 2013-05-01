<?php

import_lib('Pager');
import_lib('Table');

$table = new Table(1);
$table->addColumn(array('name' => 'name', 'title' => 'Map'));
$table->addColumn(array('name' => 'player', 'title' => 'Record holder', 'table' => 'P', 'column' => 'name_raw'));
$table->addColumn(array('name' => 'record', 'title' => 'Record', 'align' => 'right'));
$table->addColumn(array('name' => 'timestamp', 'title' => 'Date', 'align' => 'right'));

$table->processOrder('name');

$page = $hierarchy[2] - 1;
$like = search_get(3);

$pager = new Pager($page, $shared['max_rows'], "P.`id`, M.`name`, `record`, P.`name` AS `record_holder`, UNIX_TIMESTAMP(`timestamp`) AS `timestamp` FROM `map` M, `player` P WHERE P.`id`=M.`player` AND (M.`name` LIKE '%$like%' OR P.`name_raw` LIKE '%$like%')" . $table->getOrder());

$rows = $pager->getRows();
foreach ($rows as $row) {
    $table->addField(format_map($row['name']));
    $table->addField(format_player($row['record_holder'], $row['id'], -1));
    $table->addField(format_time($row['record'], $row['name']));
    $table->addField(format_date($row['timestamp']));
}

?>
<p>
Records below are the best runs recorded by the bot, not necessarily actual records.
</p>
<?= format_search($like); ?>
<?= format_pages(2, $pager); ?>
<?= $table->format(2); ?>
