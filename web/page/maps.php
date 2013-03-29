<?php

import_lib('Pager');

$pager = new Pager($hierarchy[1] - 1, $shared['max_rows'], "SELECT `name`, `record`, `record_holder` FROM `map` ORDER BY `name`");

$maps = '';
$rows = $pager->getRows();
foreach ($rows as $row) {
    $maps .= '<tr><td>' . format_map($row['name']) . '</td><td>' . format_player($row['record_holder']) . '</td><td>' . format_time($row['record']) . '</td></tr>';
}

?>
<?= format_pages(1, $pager); ?>
<table>
    <tr><th>Map</th><th>Record holder</th><th>Record</th></tr>
    <?= $maps; ?>
</table>