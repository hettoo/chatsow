<?php

define('COLUMNS', 4);

import_lib('Pager');

$players = '';
$i = 0;
$pager = new Pager($hierarchy[2] - 1, $shared['max_rows'] * COLUMNS, "SELECT `id`, `name` FROM `player` ORDER BY `last_seen` DESC");
$rows = $pager->getRows();
foreach ($rows as $row) {
	if ($i == 0)
		$players .= '<tr>';
	$players .= '<td>' . format_player($row['name'], $row['id']) . '</td>';
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
