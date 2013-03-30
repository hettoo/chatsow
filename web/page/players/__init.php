<?php

if (count($hierarchy) == 2 && $_POST['submit']) {
    header('Location: http://' . $_SERVER['HTTP_HOST'] . url('players/' . $hierarchy[1] . '/1/' . $_POST['name']));
    exit;
}

$players = '';
$result = $db->query("SELECT `id`, `record_holder` FROM `map` WHERE `id`=" . (int)$hierarchy[1]) or die($db->error);
while ($row = $result->fetch_array()) {
    $shared['head'] = 'Player ' . format_player($row['record_holder'], $row['id']);
    $shared['player'] = $row;
}

?>
