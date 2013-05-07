<?php

$files = glob($s['chatsow'] . 'demos/records/*.wd15');
if ($files) {
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
                $map = $s['db']->real_escape_string($result);
                break;
            case 2:
                $player = $s['db']->real_escape_string($result);
                break;
            case 3:
                $timestamp = (int)$result;
                break;
            }
            $state++;
        }
        fclose($fp);
        $raw_player = uncolor($player);
        $s['db']->query("INSERT INTO `player` SET `name`='$player', `name_raw`='$raw_player' ON DUPLICATE KEY UPDATE `name`=VALUES(`name`)") or die($s['db']->error);
        $result = $s['db']->query("SELECT `id` FROM `player` WHERE `name_raw`='$raw_player'") or die($s['db']->error);
        if ($row = $result->fetch_array()) {
            $id = $row['id'];
            $s['db']->query("INSERT INTO `map` SET `name`='$map', `record`=$time, `player`=$id, `timestamp`=FROM_UNIXTIME($timestamp) ON DUPLICATE KEY UPDATE `record`=VALUES(`record`), `player`=VALUES(`player`), `timestamp`=VALUES(`timestamp`)") or die($s['db']->error);
            rename($file, "./demos/$map.wd15");
        }
    }
}

?>
