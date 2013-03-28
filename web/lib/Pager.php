<?php

class Pager {
	private $page;
	private $limit;
	private $query;

	private $pages;
	private $rows;

    function __construct($page, $limit, $query) {
        $this->page = max((int)$page, 0);
        $this->limit = (int)$limit;
        $this->query = $query;
		$this->query();
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
		$result = $db->query("$this->query LIMIT $skip, 18446744073709551615") or die($db->error);
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
}

?>
