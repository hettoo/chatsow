<?php

class Search {
    private $index;

    function __construct($index) {
        $this->index = $index;
    }

    function redirect($pager) {
        global $hierarchy;
        if ($_POST['submit']) {
            $hierarchy[$pager->getIndex()] = '1';
            header('Location: http://' . $_SERVER['HTTP_HOST'] . url($_POST['name'], $this->index, false));
            exit;
        }
    }

    function get() {
        global $s, $hierarchy;
        return $s['db']->real_escape_string($hierarchy[$this->index]);
    }

    function format($pager = null) {
        global $hierarchy;
        $result = '<p>';
        $result .= '<form action="' . this_url() . '" method="POST"' . ($pager && $pager->drawable() ? ' class="left"' : '' ) . '>';
        $result .= '<input type="text" name="name" value="' . $hierarchy[$this->index] . '" />';
        $result .= '<input type="submit" name="submit" value="Search">';
        $result .= '</form>';
        if ($pager)
            $result .= $pager->format();
        $result .= '</p>';
        return $result;
    }
}

?>
