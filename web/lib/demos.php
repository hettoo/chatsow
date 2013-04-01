<?php

$files = glob($chatsow . 'demos/records/*.wd15');
foreach ($files as $file) {
    $raw = preg_replace('/\.[^\.]*$/', '', $file);
    $txt = $raw . '.txt';
    $fp = fopen($txt, 'r');
    $state = 0;
    $map = '';
    $player = '';
    $time = 0;
    $timestamp = 0;
    while (($result = fgets($fp)) !== FALSE) {
        $result = trim($result);
        switch ($state) {
        case 0:
            $time = (int)$result;
            break;
        case 1:
            $map = $db->real_escape_string($result);
            break;
        case 2:
            $player = $db->real_escape_string($result);
            break;
        case 3:
            $timestamp = (int)$result;
            break;
        }
        $state++;
    }
    fclose($fp);
    $raw_player = uncolor($player);
    $db->query("DELETE FROM `map` WHERE `name`='$map'") or die($db->error);
    $db->query("INSERT INTO `map` SET `name`='$map', `record`=$time, `record_holder`='$player', `record_holder_raw`='$raw_player', `timestamp`=FROM_UNIXTIME($timestamp)") or die($db->error);
    rename($file, "./demos/$map.wd15");
}

?>
