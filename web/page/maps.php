<?php

import_lib('Pager');

$pager = new Pager($hierarchy[1] - 1, $shared['max_rows'], "SELECT `name`, COUNT(`start`) AS `matches`, MIN(R.`time`) AS `record` FROM `map` M LEFT JOIN `match` T ON M.`name` = T.`map` LEFT JOIN `player_map_race` R ON R.`map` = `name` GROUP BY `name` ORDER BY COUNT(`start`) DESC, `name`");

$maps = '';
$rows = $pager->getRows();
foreach ($rows as $row) {
	$maps .= '<tr><td>' . $row['name'] . '</td><td>' . $row['matches'] . '</td><td>' . format_time($row['record']) . '</td></tr>';
}

?>
<?= format_pages(1, $pager); ?>
<table>
	<tr><th>Map</th><th>Matches</th><th>Record</th></tr>
	<?= $maps; ?>
</table>
