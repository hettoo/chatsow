<?php

$fp = fopen($chatsow . 'live.txt', 'r');
$players = 0;
while (($result = fgets($fp)) !== FALSE) {
    if ($players == 0) {
        echo '<h2>' . color($result) . '</h2>';
        $players = -1;
    } elseif ($players == -1) {
        $players = (int)$result;
    } else {
        echo format_player($result) . '<br/>';
        $players--;
    }
}
fclose($fp);
