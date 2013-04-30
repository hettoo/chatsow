<?php

import_lib('Pager');

$like = $db->real_escape_string($hierarchy[2]);

$pager = new Pager($hierarchy[1] - 1, $shared['max_rows'], "P.`id`, M.`name`, `record`, P.`name` AS `record_holder`, UNIX_TIMESTAMP(`timestamp`) AS `timestamp` FROM `map` M, `player` P WHERE P.`id`=M.`player` AND (M.`name` LIKE '%$like%' OR P.`name_raw` LIKE '%$like%') ORDER BY `name`");

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
<form action="<?= url('maps'); ?>" method="POST">
<input type="text" name="name" value="<?= $like; ?>" />
<input type="submit" name="submit" value="Search">
</form>
</p>
<?= format_pages(1, $pager); ?>
<table>
    <tr><th>Map</th><th>Record holder</th><th class="right">Record</th><th class="right">Date</th></tr>
    <?= $maps; ?>
</table>
