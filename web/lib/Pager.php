<?php

class Pager {
    private $index;
    private $page;
    private $limit;
    private $query;

    private $pages;
    private $rows;

    function __construct($index, $limit, $query) {
        global $hierarchy;
        $this->index = $index;
        $this->page = max((int)$hierarchy[$index] - 1, 0);
        $this->limit = (int)$limit;
        $this->query = $query;
    }

    function getIndex() {
        return $this->index;
    }

    function getOffset() {
        return $this->page * $this->limit;
    }

    function getPage() {
        return $this->page;
    }

    function query() {
        global $db;

        $skip = $this->getOffset();
        $this->rows = array();
        $result = $db->query("SELECT $this->query LIMIT $skip, 18446744073709551615") or die($db->error);
        for ($i = 0; $i < $this->limit && ($row = $result->fetch_array()); $i++)
            $this->rows[] = $row;
        $this->pages = ceil($result->num_rows / $this->limit) + $this->page;
    }

    function getRows() {
        return $this->rows;
    }

    function getPages() {
        return $this->pages;
    }

    function drawable() {
        return $this->getPages() > 1;
    }

    function format() {
        global $shared;

        if (!$this->drawable())
            return '';

        $pages = $this->getPages();
        $page = $this->getPage() + 1;
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
        $result .= '<li><a href="' . url(1, $this->index, false) . '">&lt;&lt;</a></li>';
        $result .= '<li><a href="' . url(max($page - 1, 1), $this->index, false) . '">&lt;</a></li>';
        if ($start > 1)
            $result .= '...';
        for ($i = $start; $i <= $end; $i++)
            $result .= '<li' . ($i == $page ? ' class="active"' : '') . '><a href="' . url($i, $this->index, false) . '">' . $i . '</a></li>';
        if ($end < $pages)
            $result .= '...';
        $result .= '<li><a href="' . url(min($page + 1, $pages), $this->index, false) . '">&gt;</a></li>';
        $result .= '<li><a href="' . url($pages, $this->index, false) . '">&gt;&gt;</a></li>';
        $result .= '</ul>';
        return $result;
    }
}

?>
