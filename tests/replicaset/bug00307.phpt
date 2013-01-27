--TEST--
Test for PHP-307: getHosts() turns wrong results.
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$m = mongo();


$d = $m->phpunit->bug307;
var_dump($d->findOne());

$hosts = $m->getHosts();
$host	= current($hosts);
var_dump($host);
?>
--EXPECTF--
NULL
array(6) {
	["host"]=>
	string(%s) "%s"
	["port"]=>
	int(%d)
	["health"]=>
	int(%d)
	["state"]=>
	int(%d)
	["ping"]=>
	int(%d)
	["lastPing"]=>
	int(%d)
}
