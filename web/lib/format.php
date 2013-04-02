<?php

function create_menu($level, $menu) {
    global $hierarchy;
    $result = '';
    foreach ($menu as $item) {
        $link = $item[0];
        $name = $item[1];
        $result .= '<div';
        if ($hierarchy[$level] == (empty($link) ? 'index' : $link))
            $result .= ' class="active"';
        $result .= '><a href="' . url($link, $level) . '">' . $name . '</a></div>';
    }
    return $result;
}

function color($string) {
    $result = '<span class="c7">';
    $special = false;
    $color = 7;
    for ($i = 0; $i < strlen($string); $i++) {
        $c = substr($string, $i, 1);
        switch ($c) {
        case '^':
            $special = !$special;
            if ($special)
                break;
        default:
            if ($special && ord($c) >= ord('0') && ord($c) <= ord('9')) {
                $new_color = ord($c) - ord('0');
                if ($new_color != $color && ($color >= 0 || $new_color != 7))
                {
                    if ($color >= 0)
                        $result .= '</span>';
                    $color = $new_color;
                    $result .= '<span class="c' . $color . '">';
                }
            } else {
                $result .= $c;
            }
            $special = false;
            break;
        }
    }
    if ($special)
        $result .= '^';
    if ($color >= 0)
        $result .= '</span>';
    return $result;
}

function colored($string, $url) {
    return '<span class="colored"><a href="' . url($url) . '">' . color($string) . '</a></span>';
}

function uncolor($string) {
    $result = '';
    $special = false;
    for ($i = 0; $i < strlen($string); $i++) {
        $c = substr($string, $i, 1);
        switch ($c) {
        case '^':
            $special = !$special;
            if ($special)
                break;
        default:
            if ($special && ord($c) >= ord('0') && ord($c) <= ord('9')) {
            } else {
                $result .= $c;
            }
            $special = false;
            break;
        }
    }
    if ($special)
        $result .= '^';
    return $result;
}

function format_pages($level, $pager) {
    $page = $pager->getPage() + 1;
    $pages = $pager->getPages();
    if ($pages <= 1)
        return '';

    $result = '';
    $result .= '<ul class="pager">';
    $result .= '<li><a href="' . url(1, $level, false) . '">&lt;&lt;</a></li>';
    $result .= '<li><a href="' . url(max($page - 1, 1), $level, false) . '">&lt;</a></li>';
    for ($i = 1; $i <= $pages; $i++)
        $result .= '<li' . ($i == $page ? ' class="active"' : '') . '><a href="' . url($i, $level, false) . '">' . $i . '</a></li>';
    $result .= '<li><a href="' . url(min($page + 1, $pages), $level, false) . '">&gt;</a></li>';
    $result .= '<li><a href="' . url($pages, $level, false) . '">&gt;&gt;</a></li>';
    $result .= '</ul>';
    return $result;
}

function format_player($name, $id = 0, $records = 0) {
    global $db;
    $name = htmlentities($name);
    if ($id == -1 || $records == -1) {
        $filtered = $db->real_escape_string($name);
        $result = $db->query("SELECT `id`, COUNT(`id`) AS `records` FROM `map` WHERE `record_holder`='$filtered' LIMIT 1") or die($db->error);
        if ($row = $result->fetch_array()) {
            if ($id == -1)
                $id = $row['id'];
            if ($records == -1)
                $records = $row['records'];
        }
    }
    $result = $id > 0 ? colored($name, 'players/' . $id) : color($name);
    if ($records > 0)
        $result .= ' [' . $records . ']';
    return $result;
}

function format_map($name) {
    $filtered = htmlentities($name);
    return '<a href="http://pk3.mgxrace.com/race10/basewsw/' . $name . '.pk3" target="_blank">' . $filtered . '</a>';
}

function format_map_external($name) {
    global $db;
    $result = format_map($name);
    if (file_exists("./demos/$name.wd15")) {
        $name = $db->real_escape_string($name);
        $res = $db->query("SELECT `record`, `record_holder` FROM `map` WHERE `name`='$name'") or die($db->error);
        if ($row = $res->fetch_array())
            $result .= ' (' . format_time($row['record'], $name) . ' by ' . format_player($row['record_holder'], -1) . ')';
    }
    return $result;
}

function format_date($time) {
    return '<span class="time">' . relative_time($time) . ' <span class="exacttime">' . date("j F Y @ G:i", $time == 0 ? time() : $time) . '</span></span>';
}

function format_server($server) {
    return '<a href="warsow://' . $server . '" target="_blank">' . $server . '</a>';
}

function format_time($time, $map) {
    $result = '.' . sprintf('%03d', $time % 1000);
    $time = floor($time / 1000);
    $result = sprintf('%02d', $time % 60) . $result;
    $time = floor($time / 60);
    if ($time > 0) {
        $result = sprintf('%02d', $time % 60) . ':' . $result;
        $time = floor($time / 60);
        if ($time > 0) {
            $result = $time . ':' . $result;
        }
    }
    if (!file_exists("./demos/$map.wd15"))
        return $result;
    return '<a href="' . resource_url("demos/$map.wd15") . '" target="_blank">' . $result . '</a>';
}

function format_rank($rank) {
    return ($rank + 1) . '.';
}

?>
