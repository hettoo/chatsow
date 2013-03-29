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
    while (($result = fgets($fp)) !== FALSE) {
        $result = trim($result);
        switch ($state) {
        case 0:
            $map = mysql_real_escape_string($result);
            break;
        case 1:
            $player = mysql_real_escape_string($result);
            break;
        case 2:
            $time = (int)$result;
            break;
        }
        $state++;
    }
    fclose($fp);
    $db->query("DELETE FROM `map` WHERE `name`='$map'") or die($db->error);
    $db->query("INSERT INTO `map` SET `name`='$map', `record`=$time, `record_holder`='$player'") or die($db->error);
    rename($file, "./demos/$map.wd15");
    unlink($txt);
}

?>
