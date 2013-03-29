<?php

$shared['submenu'] = '';
init_page($args);
$main_menu = create_menu(0, array(
	array('', 'Home'),
	array('maps', 'Maps'),
	array('players', 'Players'),
	array('live', 'Live')
));

if (empty($shared['head']))
	$shared['head'] = 'Unnamed page';

?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
		<title><?= preg_replace('/<[^>]*>/', '', $shared['head']); ?></title>
		<style>
			body {
				font-family: Sans, Arial, Verdana;
				font-size: 12px;
				background-color: #24202d;
				color: #fff;
			}

			a {
				text-decoration: none;
				color: #090;
			}
			a:hover {
				color: #070;
			}
			h1 {
				padding: 0px;
				margin: 0px 0px 10px 0px;
			}
            h2 span.sub {
                display: block;
                font-size: 12px;
            }
			th {
				text-align: left;
			}
			th, td {
				padding-right: 20px;
			}
			li.active {
				background-color: #090;
			}
			li.active a {
				color: #fff;
			}

			.clear {
				clear: both;
			}

			#main {
				width: 960px;
			}

			#first, #sub-menu {
				float: left;
				width: 128px;
				overflow: hidden;
			}
			#main-menu, #head, #content {
				margin-left: 128px;
			}
			#head, #content {
				width: 620px;
				padding-left: 24px;
			}
			#content {
				padding-bottom: 20px;
			}
			#main-menu div {
				float: left;
				padding: 10px;
				border-top: 1px solid black;
				border-bottom: 1px solid black;
				font-weight: bold;
				font-size: 16px;
				margin-bottom: 24px;
			}
			#main-menu .active, #sub-menu .active {
				background-color: #090;
			}
			#main-menu .active a, #sub-menu .active a {
				color: #fff;
			}

			#sub-menu div {
				font-weight: bold;
				font-size: 14px;
				padding: 10px 22px 10px 0px;
				text-align: right;
			}

			ul.pager {
				list-style-type: none;
				margin: 8px 0;
				padding: 0;
			}
			ul.pager li {
				display: inline;
				padding: 0 5px;
			}

			.colored a:hover {
				border-bottom: 1px dotted #090;
			}

			.c0 {
				color: black;
			}
			.c1 {
				color: red;
			}
			.c2 {
				color: green;
			}
			.c3 {
				color: yellow;
			}
			.c4 {
				color: #25f;
			}
			.c5 {
				color: cyan;
			}
			.c6 {
				color: magenta;
			}
			.c7 {
				color: white;
			}
			.c8 {
				color: orange;
			}
			.c9 {
				color: grey;
			}

			.time {
				position: relative;
			}
			.time .exacttime {
				display: none;
			}
			.time:hover .exacttime {
				position: absolute;
				top: -16px;
				width: 200px;
				display: block;
				background-color: #000;
			}
		</style>
</head>
<body>
		<div id="main">
			<div id="header">
			</div>
			<div id="first">
			</div>
			<div id="main-menu">
				<?= $main_menu; ?>
			</div>
			<div class="clear"></div>
			<div id="head">
				<h1><?= empty($shared['head']) ? 'Unnamed page' : $shared['head']; ?></h1>
			</div>
			<div id="sub-menu">
				<?= $shared['submenu']; ?>
			</div>
			<div id="content">
				<?php import_page($args); ?>
			</div>
		</div>
</body>
</html>
