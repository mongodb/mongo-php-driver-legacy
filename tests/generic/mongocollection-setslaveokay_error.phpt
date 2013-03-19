--TEST--
MongoCollection::setSlaveOkay()
--DESCRIPTION--
Test for a value that cannot convert to boolean
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();
$c = $m->phpunit->col;
var_dump($c->setSlaveOkay(array('error')));
?>
--EXPECTF--
%s: Function MongoCollection::setSlaveOkay() is deprecated in %s on line %d

Warning: MongoCollection::setSlaveOkay() expects parameter 1 to be boolean, array given in %smongocollection-setslaveokay_error.php on line %d
NULL
