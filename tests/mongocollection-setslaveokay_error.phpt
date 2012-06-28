--TEST--
MongoCollection::setSlaveOkay()
--DESCRIPTION--
Test for a value that cannot convert to boolean
--FILE--
<?php
$m = new Mongo();
$c = $m->phpunit->col;
var_dump($c->setSlaveOkay(array('error')));
?>
--EXPECTF--
Warning: MongoCollection::setSlaveOkay() expects parameter 1 to be boolean, array given in %smongocollection-setslaveokay_error.php on line 4
NULL

