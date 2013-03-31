<?php

define('COLUMNS', 4);

import_lib('Pager');

$players = '';
$i = 0;
$pager = new Pager($hierarchy[2] - 1, $shared['max_rows'] * COLUMNS, "SELECT `id`, `record_holder`, COUNT(`record_holder`) AS `records` FROM `map` GROUP BY `record_holder` ORDER BY COUNT(`record_holder`) DESC, `record_holder`");
$rows = $pager->getRows();
foreach ($rows as $row) {
	if ($i == 0)
		$players .= '<tr>';
	$players .= '<td>' . format_player($row['record_holder'], $row['id'], $row['records']) . '</td>';
	$i = ($i + 1) % COLUMNS;
	if ($i == 0)
		$players .= '</tr>';
}
if ($i != 0)
	$players .= '</tr>';

?>

<?= format_pages(2, $pager); ?>
<table>
<?= $players; ?>
</table>
