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

function format_player($name, $id = 0) {
	$name = htmlentities($name);
	return $id > 0 ? colored($name, 'players/' . $id) : color($name);
}

function format_map($name) {
    return '<a href="' . resource_url("demos/$name.wd15") . '">' . htmlentities($name) . '</a>';
}

function format_date($time) {
	return '<span class="time">' . relative_time($time) . ' <span class="exacttime">' . date("j F Y @ G:i", $time == 0 ? time() : $time) . '</span></span>';
}

function format_time($time) {
	$result = '.' . ($time % 1000);
	$time = floor($time / 1000);
	$result = ($time % 60) . $result;
	$time = floor($time / 60);
	if ($time > 0) {
		$result = ($time % 60) . ':' . $result;
		$time = floor($time / 60);
		if ($time > 0) {
			$result = $time . ':' . $result;
		}
	}
	return $result;
}

function format_rank($rank) {
	return ($rank + 1) . '.';
}

?>
