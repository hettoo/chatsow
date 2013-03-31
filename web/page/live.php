<p>
The bot is currently connected to the following servers.
</p>
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
            echo '<h2>' . color($result);
            $state++;
            break;
        case 1:
            echo '<span class="sub">' . format_server($result);
            $state++;
            break;
        case 2:
            echo '<br />Map: ' . format_map_external($result) . '</span></h2>';
            $state++;
            break;
        case 3:
            echo format_player($result, -1, -1) . '<br />';
            break;
        }
    }
}
fclose($fp);
