<?php

import_lib('Table');

$s['head'] = 'Live';
$s['description'] = 'The servers the bot is currently connected to, along with their players.';

$result = '';
$fp = fopen($s['chatsow'] . 'live.txt', 'r');
$live = null;
if ($fp) {
    $state = 0;
    while (($result = fgets($fp)) !== FALSE) {
        $result = trim($result);
        if ($result == '') {
            $state = 0;
        } else {
            switch ($state) {
            case 0:
                if (isset($table))
                    $live .= $table->format();
                $live .= '<h2>' . color($result);
                $state++;
                break;
            case 1:
                $live .= '<span class="sub">Location: ' . format_server($result);
                $state++;
                break;
            case 2:
                $live .= '<br />Map: ' . format_map_external($result) . '</span></h2>';

                $table = new Table();
                $table->setEmptyMessage('No players connected to this server. :(');
                $table->addColumn(array('title' => 'Player', 'size' => 'large'));
                $table->addColumn(array('title' => 'Score', 'size' => 'small'));
                $table->addColumn(array('title' => 'Team', 'size' => 'medium'));

                $state++;
                break;
            case 3:
                $table->addField(format_player($result, -1, -1));
                $table->addField(trim(fgets($fp)));
                $table->addField(team_name(trim(fgets($fp))));
                break;
            }
        }
    }
    fclose($fp);
}
if (isset($table))
    $live .= $table->format();

$s['live'] = $live;

?>
