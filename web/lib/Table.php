<?php

class Table {
    private $columns;
    private $order_index;
    private $pager;
    private $head;
    private $force_columns;

    private $content;
    private $x;

    private $column;
    private $table;
    private $descending;

    function __construct($order_index = null) {
        $this->content = '';
        $this->x = 0;
        $this->pager = null;
        $this->columns = array();
        $this->order_index = $order_index;
        $this->head = true;
        $this->force_columns = 1;
    }

    function forceColumns($amount) {
        $this->head = false;
        $this->force_columns = $amount;
    }

    function addColumn($values) {
        if (!isset($values['column']))
            $values['column'] = $values['name'];
        $this->columns[] = $values;
    }

    function getOrdering($default = null) {
        global $db, $hierarchy;
        $order = $db->real_escape_string($hierarchy[$this->order_index]);
        if (isset($default)) {
            if ($order == '')
                $order = 'name';
            $hierarchy[$this->order_index] = $order;
        }
        $descending = substr($order, 0, 1) == '-';
        if ($descending)
            $order = substr($order, 1);
        return array($order, $descending);
    }

    function processOrder($default_order = null) {
        $table = '';
        list($order, $this->descending) = $this->getOrdering($this->order_index, $default_order);
        foreach ($this->columns as $values) {
            if ($values['name'] == $order) {
                $this->column = $values['column'];
                $this->table = $values['table'];
            }
        }
    }

    function getOrder() {
        return ' ORDER BY ' . (isset($this->table) ? $this->table . '.' : '') . '`' . $this->column . '` ' . ($this->descending ? 'DESC' : 'ASC');
    }

    function addField($value) {
        if ($this->x == 0)
            $this->content .= '<tr>';
        $this->content .= '<td';
        if ($this->head && isset($this->columns[$this->x]['align']))
            $this->content .= ' class="' . $this->columns[$this->x]['align'] . '"';
        $this->content .= '>';
        $this->content .= $value;
        $this->content .= '</td>';
        $this->x = ($this->x + 1) % ($this->head ? count($this->columns) : $this->force_columns);
        if ($this->x == 0)
            $this->content .= '</tr>';
    }

    function setPager($pager) {
        $this->pager = $pager;
    }

    function invert($link) {
        global $hierarchy;
        $current = $hierarchy[$this->order_index];
        if ($current == $link)
            return '-' . $link;
        return $link;
    }

    function prefix($link) {
        global $hierarchy;
        $current = $hierarchy[$this->order_index];
        if ($current == $link)
            return '+';
        if ($current == '-' . $link)
            return '-';
        return '';
    }

    function format() {
        global $hierarchy;
        $result = '';
        if (isset($this->pager)) {
            $result .= $this->pager->format();
            $index = $this->pager->getIndex();
            $page = $hierarchy[$index];
            $hierarchy[$index] = '1';
        }
        $result .= '<table>';
        if ($this->head) {
            $result .= '<tr>';
            foreach ($this->columns as $values) {
                $result .= '<th';
                if (isset($values['align']))
                    $result .= ' class="' . $values['align'] . '"';
                $result .= '>';
                if (isset($this->order_index)) {
                    $result .= '<a href="' . url($this->invert($values['name']), $this->order_index, false) . '">';
                    $result .= $this->prefix($values['name']);
                }
                $result .= $values['title'];
                if (isset($this->order_index))
                    $result .= '</a>';
                $result .= '</th>';
            }
            $result .= '</tr>';
        }
        if (isset($this->pager))
            $hierarchy[$this->pager->getIndex()] = $page;
        $result .= $this->content;
        if ($this->x != 0)
            $result .= '</tr>';
        $result .= '</table>';
        return $result;
    }
}

?>
