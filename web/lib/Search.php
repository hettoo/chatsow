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
        global $db, $hierarchy;
        return $db->real_escape_string($hierarchy[$this->index]);
    }

    function format() {
        global $hierarchy;
        $result = '<p>';
        $result .= '<form action="' . this_url() . '" method="POST">';
        $result .= '<input type="text" name="name" value="' . $hierarchy[$this->index] . '" />';
        $result .= '<input type="submit" name="submit" value="Search">';
        $result .= '</form>';
        $result .= '</p>';
        return $result;
    }
}

?>
