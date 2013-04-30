<?php

function plural($num) {
    if ($num != 1)
        return 's';
}

function relative_time($time) {
    $diff = time() - $time;
    if ($time == 0)
        return 'unknown';

    if ($diff == 0)
        return 'now';

    if ($diff < 60)
        return $diff . ' second' . plural($diff) . ' ago';
    $diff = round($diff / 60);

    if ($diff < 60)
        return $diff . ' minute' . plural($diff) . ' ago';
    $diff = round($diff / 60);

    if ($diff < 24)
        return $diff . ' hour' . plural($diff) . ' ago';
    $diff = round($diff / 24);

    if ($diff < 7)
        return $diff . ' day' . plural($diff) . ' ago';
    $diff = round($diff / 7);

    if ($diff < 4)
        return $diff . ' week' . plural($diff) . ' ago';
    $diff = round($diff / 4);

    if ($diff < 24)
        return $diff . ' month' . plural($diff) . ' ago';
    $diff = round($diff / 12);

    return $diff . ' year' . plural($diff) . ' ago';
}

?>
