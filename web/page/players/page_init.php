<?php

$shared['head'] = 'Players';
$shared['description'] = 'Racesow player list ordered by the amount of record demos they own.';

define('COLUMNS', 4);

import_lib('Pager');
import_lib('Table');

$i = 0;
$pager = new Pager(2, $shared['max_rows'] * COLUMNS, "P.`id`, P.`name`, COUNT(*) AS `records` FROM `map` M, `player` P WHERE M.`player`=P.`id` GROUP BY P.`name_raw` ORDER BY COUNT(*) DESC, P.`name`");

$table = new Table();
$table->forceColumns(COLUMNS);

$pager->query();
$rows = $pager->getRows();
foreach ($rows as $row)
    $table->addField(format_player($row['name'], $row['id'], $row['records']));

$shared['pager'] = $pager;
$shared['table'] = $table;

?>
