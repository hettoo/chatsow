<?php

import_lib('Pager');

list($order, $descending) = get_order(1, 'name');
$page = $hierarchy[2] - 1;
$like = $db->real_escape_string($hierarchy[3]);

$pager = new Pager($page, $shared['max_rows'], "P.`id`, M.`name`, `record`, P.`name` AS `record_holder`, UNIX_TIMESTAMP(`timestamp`) AS `timestamp` FROM `map` M, `player` P WHERE P.`id`=M.`player` AND (M.`name` LIKE '%$like%' OR P.`name_raw` LIKE '%$like%') ORDER BY `$order`" . ($descending ? 'DESC' : 'ASC'));

$maps = '';
$rows = $pager->getRows();
foreach ($rows as $row) {
    $maps .= '<tr><td>' . format_map($row['name']) . '</td><td>' . format_player($row['record_holder'], $row['id'], -1) . '</td><td class="right">' . format_time($row['record'], $row['name']) . '</td><td class="right">' . format_date($row['timestamp']) . '</td></tr>';
}

?>
<p>
Records below are the best runs recorded by the bot, not necessarily actual records.
</p>
<p>
<form action="<?= this_url(); ?>" method="POST">
<input type="text" name="name" value="<?= $like; ?>" />
<input type="submit" name="submit" value="Search">
</form>
</p>
<?= format_pages(2, $pager); ?>
<table>
    <?= format_head(1, 2, array('Map' => 'name', 'Record holder' => 'player'), array('Record' => 'record', 'Date' => 'timestamp')); ?>
    <?= $maps; ?>
</table>
