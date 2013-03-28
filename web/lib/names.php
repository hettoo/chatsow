<?php

function team_name($team) {
	switch ($team) {
	case 0:
		return 'SPECTATORS';
	case 1:
		return 'PLAYERS';
	case 2:
		return 'ALPHA';
	case 3:
		return 'BETA';
	default:
		return 'UNKNOWN TEAM';
	}
}

function state_name($state) {
	switch ($state) {
	case 1:
		return 'WARMUP';
	case 2:
		return 'COUNTDOWN';
	case 3:
		return 'PLAYTIME';
	case 4:
		return 'POSTMATCH';
	case 5:
		return 'WAITEXIT';
	default:
		return 'UNKNOWN STATE';
	}
}

?>
