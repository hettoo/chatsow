<?php

$maps = '';
$result = $db->query("SELECT `id`, `name`, `record`, `record_holder` FROM `map` ORDER BY `timestamp` DESC, `name` LIMIT 8") or die($db->error);
while ($row = $result->fetch_array())
    $maps .= '<tr><td>' . format_map($row['name']) . '</td><td>' . format_player($row['record_holder'], $row['id'], -1) . '</td><td align="right">' . format_time($row['record']) . '</td></tr>';

?>
<p>
This is the webinterface of an instance of the <a href="http://github.com/hettoo/chatsow" target="_blank">chatsow</a> project, created by <?= format_player('^7^0/^7inc^2.^7hettoo^0/', -1); ?>.
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
<h2>Latest records</h2>
<p>
Records below are the best runs recorded by the bot, not necessarily actual records.
</p>
<table>
    <tr><th>Map</th><th>Record holder</th><th>Record</th></tr>
    <?= $maps; ?>
</table>
