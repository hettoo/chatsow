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

function format_search($current) {
    $result = '<p>';
    $result .= '<form action="' . this_url() . '" method="POST">';
    $result .= '<input type="text" name="name" value="' . $current . '" />';
    $result .= '<input type="submit" name="submit" value="Search">';
    $result .= '</form>';
    $result .= '</p>';
    return $result;
}

function format_pages($level, $pager) {
    global $shared;

    $pages = $pager->getPages();
    if ($pages <= 1)
        return '';

    $page = $pager->getPage() + 1;
    $start = 1;
    $end = $pages;
    $max_left = floor(($shared['max_pages'] - 1) / 2);
    $max_right = ceil(($shared['max_pages'] - 1) / 2);
    $fit_left = $page - $start;
    $fit_right = $end - $page;
    $missed_left = max(0, $max_left - $fit_left);
    $missed_right = max(0, $max_right - $fit_right);
    $left = min($fit_left, $max_left + $missed_right);
    $right = min($fit_right, $max_right + $missed_left);
    $start = $page - $left;
    $end = $page + $right;

    $result = '';
    $result .= '<ul class="pager">';
    $result .= '<li><a href="' . url(1, $level, false) . '">&lt;&lt;</a></li>';
    $result .= '<li><a href="' . url(max($page - 1, 1), $level, false) . '">&lt;</a></li>';
    if ($start > 1)
        $result .= '...';
    for ($i = $start; $i <= $end; $i++)
        $result .= '<li' . ($i == $page ? ' class="active"' : '') . '><a href="' . url($i, $level, false) . '">' . $i . '</a></li>';
    if ($end < $pages)
        $result .= '...';
    $result .= '<li><a href="' . url(min($page + 1, $pages), $level, false) . '">&gt;</a></li>';
    $result .= '<li><a href="' . url($pages, $level, false) . '">&gt;&gt;</a></li>';
    $result .= '</ul>';
    return $result;
}

function format_player($name, $id = 0, $records = 0) {
    global $db;
    if ($id == -1 || $records == -1) {
        $filtered = $db->real_escape_string(uncolor($name));
        $result = $db->query("SELECT P.`id`, COUNT(*) AS `records` FROM `map` M, `player` P WHERE P.`name_raw`='$filtered' AND M.`player`=P.`id` LIMIT 1") or die($db->error);
        if ($row = $result->fetch_array()) {
            if ($id == -1)
                $id = $row['id'];
            if ($records == -1)
                $records = $row['records'];
        }
    }
    $name = htmlentities($name);
    $result = $id > 0 ? colored($name, 'players/' . $id) : color($name);
    if ($records > 0)
        $result .= ' [' . $records . ']';
    return $result;
}

function format_map($name) {
    $filtered = htmlentities($name);
    return '<a href="http://pk3.mgxrace.net/racesow/' . $name . '.pk3" target="_blank">' . $filtered . '</a>';
}

function format_map_external($name) {
    global $db;
    $result = format_map($name);
    if (file_exists("./demos/$name.wd15")) {
        $name = $db->real_escape_string($name);
        $res = $db->query("SELECT `record`, P.`name` FROM `map` M, `player` P WHERE M.`name`='$name' AND P.`id`=M.`player`") or die($db->error);
        if ($row = $res->fetch_array())
            $result .= ' (' . format_time($row['record'], $name) . ' by ' . format_player($row['name'], -1) . ')';
    }
    return $result;
}

function format_date_relative($time) {
    return '<span class="time">' . relative_time($time) . ($time == 0 ? '' : ' <span class="exacttime">' . exact_datetime($time) . '</span>') . '</span>';
}

function format_date($time) {
    return '<span class="time">' . exact_date($time) . ($time == 0 ? '' : ' <span class="exacttime">' . exact_time($time) . '</span>') . '</span>';
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
