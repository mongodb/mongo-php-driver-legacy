--TEST--
Test for bug PHP-307: getHosts() turns wrong results.
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$m = rs();


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
