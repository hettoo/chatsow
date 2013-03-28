<?php

$players = '';
$result = $db->query("SELECT `key`, `name`, UNIX_TIMESTAMP(`first_seen`) AS `first_seen`, UNIX_TIMESTAMP(`last_seen`) AS `last_seen` FROM `player` WHERE `id`=" . (int)$hierarchy[1]) or die($db->error);
while ($row = $result->fetch_array()) {
	$shared['head'] = 'Player ' . format_player($row['name']);
	if (!isset($row['last_seen']))
		$row['last_seen'] = $row['first_seen'];
	$shared['player'] = $row;
}
$result = $db->query("SELECT COUNT(*) AS `count` FROM `live` WHERE `key`='{$shared['player']['key']}'") or die($db->error);
while ($row = $result->fetch_array()) {
	if ($row['count'] > 0)
		$shared['player']['last_seen'] = 0;
}

?>
