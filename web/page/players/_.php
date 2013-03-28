<?php

$stats = '';
$result = $db->query("SELECT P.`id`, `name`, SUM(`score`) AS `score`, SUM(`frags`) AS `frags`, SUM(`deaths`) AS `deaths` FROM `player_stats_normal` S INNER JOIN `player` P ON S.`key` = P.`key` WHERE S.`key` = '{$shared['player']['key']}' GROUP BY S.`key` ORDER BY `score`, `frags`, `deaths` ASC") or die($db->error);
while ($row = $result->fetch_array()) {
	$stats .= '<tr><td>Overall</td><td>' . $row['score'] . '</td><td>' . $row['frags'] . '</td><td>' . $row['deaths'] . '</td><td>' . round($row['frags'] / $row['deaths'], 2) . '</td></tr>';
}
$result = $db->query("SELECT `gametype`, `score`, `frags`, `deaths` FROM `player_stats_normal` WHERE `key` = '{$shared['player']['key']}' ORDER BY `score`, `frags`, `deaths` ASC") or die($db->error);
while ($row = $result->fetch_array()) {
	$stats .= '<tr><td>' . format_gametype($row['gametype']) . '</td><td>' . $row['score'] . '</td><td>' . $row['frags'] . '</td><td>' . $row['deaths'] . '</td><td>' . round($row['frags'] / $row['deaths'], 2) . '</td></tr>';
}
$spree = '';
$result = $db->query("SELECT `spree` FROM `spree` WHERE `key` = '{$shared['player']['key']}'") or die($db->error);
while ($row = $result->fetch_array()) {
	$spree = '<br /><a href="' . url('stats/sprees') . '">Spree record</a>: ' . $row['spree'] . ' frags in a row';
}
$awards = '';
$result = $db->query("SELECT `id`, `award`, `count` FROM `award` WHERE `key` = '{$shared['player']['key']}' ORDER BY `count`") or die($db->error);
while ($row = $result->fetch_array()) {
	$awards .= '<tr><td>' . format_award($row['award'], $row['id']) . '</td><td>' . $row['count'] . '</td></tr>';
}

?>
<p>
First seen: <?= format_date($shared['player']['first_seen']); ?><br />
Last seen: <?= format_date($shared['player']['last_seen']); ?>
<?= $spree; ?>
</p>

<h2>Stats</h2>
<table>
	<tr><th>Gametype</th><th>Score</th><th>Frags</th><th>Deaths</th><th>F/D</th></tr>
	<?= $stats; ?>
</table>

<h2>Awards</h2>
<table>
	<tr><th>Award</th><th>Count</th></tr>
	<?= $awards; ?>
</table>
