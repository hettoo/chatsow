<?php

global $db;
$db = new mysqli($s['host'], $s['user'], $s['password'], $s['database']) or die("Unable to connect to the database.");

?>
