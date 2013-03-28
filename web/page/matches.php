<?php

import_lib('Pager');

$pager = new Pager($hierarchy[1] - 1, $shared['max_rows'], "SELECT `server`, `gametype`, `map`, TIMEDIFF(`end`, `start`) AS `time`, `end_state` FROM `match` WHERE `end` IS NOT NULL ORDER BY `end` DESC, `start` DESC");

$matches = '';
$rows = $pager->getRows();
foreach ($rows as $row) {
	$matches .= '<tr><td>' . format_server($row['server']) . '</td><td>' . format_gametype($row['gametype']) . '</td><td>' . $row['map'] . '</td><td>' . $row['time'] . '</td><td>' . state_name($row['end_state']) . '</td></tr>';
}

?>
<?= format_pages(1, $pager); ?>
<table>
	<tr><th>Server</th><th>Gametype</th><th>Map</th><th>Playtime</th><th>End state</th></tr>
	<?= $matches; ?>
</table>
