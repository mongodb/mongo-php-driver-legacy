<?php
require_once dirname(__FILE__) . '/skipif.inc';

$status = $m->selectDB('phpunit')->command(array('serverStatus' => 1));

if ('mongos' === $status['process']) {
	die('skip This test is not compatible with mongos');
}
