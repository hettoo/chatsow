<?php

class Table {
    private $columns;
    private $order_index;

    private $content;
    private $x;

    private $column;
    private $table;
    private $descending;

    function __construct($order_index) {
        $this->content = '';
        $this->x = 0;
        $this->columns = array();
        $this->order_index = $order_index;
    }

    function addColumn($values) {
        if (!isset($values['column']))
            $values['column'] = $values['name'];
        $this->columns[] = $values;
    }

    function processOrder($default_order) {
        $table = '';
        list($order, $this->descending) = get_order($this->order_index, $default_order);
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
        if (isset($this->columns[$this->x]['align']))
            $this->content .= ' class="' . $this->columns[$this->x]['align'] . '"';
        $this->content .= '>';
        $this->content .= $value;
        $this->content .= '</td>';
        $this->x = ($this->x + 1) % count($this->columns);
        if ($this->x == 0)
            $this->content .= '</tr>';
    }

    function format($page_index) {
        global $hierarchy;
        $result = '<table>';
        $page = $hierarchy[$page_level];
        $hierarchy[$page_index] = '1';
        $result .= '<tr>';
        foreach ($this->columns as $values) {
            $result .= '<th';
            if (isset($values['align']))
                $result .= ' class="' . $values['align'] . '"';
            $result .= '>';
            $result .= '<a href="' . url(invert_search($this->order_index, $values['name']), $this->order_index, false) . '">' . search_prefix($this->order_index, $values['name']) . $values['title'] . '</a>';
            $result .= '</th>';
        }
        $result .= '</tr>';
        $hierarchy[$page_level] = $page;
        $result .= $this->content;
        $result .= '</table>';
        return $result;
    }
}

?>
