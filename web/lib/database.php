<?php

global $db;
$db = new mysqli($shared['host'], $shared['user'], $shared['password'], $shared['database']) or die("Unable to connect to the database.");

?>
