<?php

define('COLUMNS', 4);

import_lib('Pager');

$players = '';
$i = 0;
$pager = new Pager(2, $shared['max_rows'] * COLUMNS, "P.`id`, P.`name`, COUNT(*) AS `records` FROM `map` M, `player` P WHERE M.`player`=P.`id` GROUP BY P.`name_raw` ORDER BY COUNT(*) DESC, P.`name`");

$pager->query();
$rows = $pager->getRows();
foreach ($rows as $row) {
	if ($i == 0)
		$players .= '<tr>';
	$players .= '<td>' . format_player($row['name'], $row['id'], $row['records']) . '</td>';
	$i = ($i + 1) % COLUMNS;
	if ($i == 0)
		$players .= '</tr>';
}
if ($i != 0)
	$players .= '</tr>';

?>

<?= $pager->format(); ?>
<table>
<?= $players; ?>
</table>
