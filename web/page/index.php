<?php

$maps = '';
$result = $db->query("SELECT `id`, `name`, `record`, `record_holder` FROM `map` ORDER BY `timestamp` DESC, `name` LIMIT 8") or die($db->error);
while ($row = $result->fetch_array())
    $maps .= '<tr><td>' . format_map($row['name']) . '</td><td>' . format_player($row['record_holder'], $row['id'], -1) . '</td><td>' . format_time($row['record']) . '</td></tr>';

?>
<p>
This is the webinterface of an instance of the <a href="http://github.com/hettoo/chatsow" target="_blank">chatsow</a> project, created by <?= format_player('^7^0/^7inc^2.^7hettoo^0/', -1); ?>.
</p>
<h2>Latest records</h2>
<p>
Records below are the best runs recorded by the bot, not necessarily actual records.
</p>
<table>
    <tr><th>Map</th><th>Record holder</th><th>Record</th></tr>
    <?= $maps; ?>
</table>
