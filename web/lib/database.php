<?php

global $db;
$db = new mysqli($host, $user, $password, $database) or die("Unable to connect to the database.");

?>
