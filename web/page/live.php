<?php

$fp = fopen($chatsow . 'live.txt', 'r');
$state = 0;
while (($result = fgets($fp)) !== FALSE) {
    $result = trim($result);
    if ($result == '') {
        $state = 0;
    } else {
        switch ($state) {
        case 0:
            echo '<h2>' . color($result) . '</h2>';
            $state++;
            break;
        case 1:
            echo '<b>Map:</b> ' . format_map($result) . '<br /><br />';
            $state++;
            break;
        case 2:
            echo format_player($result) . '<br />';
            break;
        }
    }
}
fclose($fp);
