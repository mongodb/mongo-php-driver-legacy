--TEST--
MongoCollection::setSlaveOkay()
--DESCRIPTION--
Test for a value that cannot convert to boolean
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$m = mongo();
$c = $m->phpunit->col;
var_dump($c->setSlaveOkay(array('error')));
?>
--EXPECTF--
Warning: MongoCollection::setSlaveOkay() expects parameter 1 to be boolean, array given in %smongocollection-setslaveokay_error.php on line %d
NULL

