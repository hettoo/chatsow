<?php

$maps = '';
$result = $db->query("SELECT P.`id`, M.`name`, `record`, P.`name` AS `record_holder`, UNIX_TIMESTAMP(`timestamp`) AS `timestamp` FROM `map` M, `player` P WHERE P.`id`=M.`player` ORDER BY `timestamp` DESC LIMIT 8") or die($db->error);
while ($row = $result->fetch_array())
    $maps .= '<tr><td>' . format_map($row['name']) . '</td><td>' . format_player($row['record_holder'], $row['id'], -1) . '</td><td class="right">' . format_time($row['record'], $row['name']) . '</td><td>' . format_date($row['timestamp']) . '</td></tr>';

$map_count = 0;
$result = $db->query("SELECT COUNT(*) AS `maps` FROM `map`") or die($db->error);
while ($row = $result->fetch_array())
    $map_count = $row['maps'];
$player_count = 0;
$result = $db->query("SELECT COUNT(*) AS `players` FROM `player`") or die($db->error);
while ($row = $result->fetch_array())
    $player_count = $row['players'];

?>
<p>
This is the webinterface of an instance of the <a href="http://github.com/hettoo/chatsow" target="_blank">chatsow</a> project created by <?= format_player('^7^0/^7inc^2.^7hettoo^0/', -1); ?>, a <a href="http://warsow.net" target="_blank">Warsow</a> chat client with specific plugin functionality for <a href="http://github.com/Racenet/racesow" target="_blank">Racesow</a>.
</p>
<p>
Some demos could still cause an error or contain the wrong run or POV.
For example, when the 'new server record' message comes more than 8 seconds after you respawn after the run, the wrong run will be saved.
</p>
<p>
Some players believe that their records should not be shared.
In my opinion, race should be a collaboration of players trying to approach the optimal time for a map together, not a place where selfish people desperately try to suppress competition.
Eventually routes will be stolen anyway, if people really want to do this.
Play on your own server, disconnected from the rest of the world, if you want to be sure.
</p>
<h2>Stats</h2>
<p>
A total of <b><?= $map_count; ?></b> records have been recorded from <b><?= $player_count; ?></b> different players.
</p>
<h2>Latest records</h2>
<p>
Records below are the best runs recorded by the bot, not necessarily actual records.
</p>
<table>
    <tr><th>Map</th><th>Record holder</th><th class="right">Record</th><th>Date</th></tr>
    <?= $maps; ?>
</table>
