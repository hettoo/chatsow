<?php

if (count($hierarchy) == 1 && $_POST['submit']) {
    header('Location: http://' . $_SERVER['HTTP_HOST'] . url('maps/1/' . $_POST['name']));
    exit;
}

$shared['head'] = 'Maps';

?>
