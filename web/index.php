<?php

function script($script) {
    return $script . '.php';
}

function import_once($script) {
    global $base, $args, $hierarchy, $db;
    global $shared;
    include_once(script($script));
}

function import($script) {
    global $base, $args, $hierarchy, $db;
    global $shared;
    include(script($script));
}

function import_lib($lib) {
    import_once("lib/$lib");
}

function page_file($page) {
    $page[0] = '/';
    return 'page' . $page;
}

function real_page($page) {
    global $args;
    while ($page != $args[0] && !file_exists(script(page_file($page)))) {
        $next_page = dirname($page);
        $page = $next_page . '/_';
        if (!file_exists(script(page_file($page))))
            $page = $next_page;
    }
    if ($page == $args[0])
        $page .= '404';
    return $page;
}

function init_child_page($child) {
    global $args;
    init_page(real_page($args) . '/' . $child);
}

function import_child_page($child, $header = true) {
    global $args, $shared, $hierarchy;
    $page = real_page($args) . '/' . $child;
    $hierarchy[1] = $child;
    if ($header) {
        init_page($page);
        echo '<h2>' . $shared['head'] . '</h2>';
    }
    import_page($page);
}

function resource_url($target) {
    global $base;
    return $base . $target;
}

function url($target, $level = 0, $rootify = true) {
    global $base, $args, $hierarchy;
    $result = $base . $args[0];
    for ($i = 0; $i < count($hierarchy); $i++) {
        if ($i > 0)
            $result .= '/';
        if ($i == $level) {
            $result .= $target;
            if ($rootify)
                return $result;
        } else {
            $result .= $hierarchy[$i];
        }
    }
    if ($i <= $level) {
        if ($i > 0)
            $result .= '/';
        $result .= $target;
    }
    return $result;
}

function this_url() {
    global $hierarchy;
    return url(join('/', $hierarchy));
}

function init_page($page) {
    $file = page_file(real_page($page) . '_init');
    if (file_exists(script($file)))
        import_once($file);
}

function import_page($page) {
    import(page_file(real_page($page)));
}

$base = $_GET['base'];
$args = $_GET['args'];

if (!isset($base)) {
    $request = $_SERVER['REQUEST_URI'];
    $request = preg_replace('/[^\/]*.php$/', '', $request);
    if (empty($request)) {
        $request = '/';
    }
    header('Location: http://' . $_SERVER['HTTP_HOST'] . $request . '/');
    exit;
}

$args = preg_replace('/\/$/', '', $args);

if (empty($args))
    $args = '/index';

$shared = array(
    'max_rows' => 20,
    'max_pages' => 10
);
$hierarchy = explode('/', substr($args, 1));

include_once('config.php');

import_lib('names');
import_lib('format');
import_lib('time');
import_lib('database');
import_lib('demos');
import_lib('content');

?>
