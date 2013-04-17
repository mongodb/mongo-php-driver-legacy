--TEST--
Test for PHP-307: getHosts() turns wrong results.
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo();


$d = $m->phpunit->bug307;
var_dump($d->findOne());

$hosts = $m->getHosts();
$host  = current($hosts);
var_dump($host);
?>
--EXPECTF--
NULL
array(6) {
  ["host"]=>
  string(%d) "%s"
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
