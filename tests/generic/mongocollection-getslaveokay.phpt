--TEST--
MongoCollection::getSlaveOkay()
--DESCRIPTION--
Test both true and false for slave_okay attribute
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require dirname(__FILE__) . "/../utils.inc";
$m = mongo();

$c = $m->phpunit->col;
$c->setSlaveOkay((bool) array('foo'));
var_dump($c->getSlaveOkay());
$c->setSlaveOkay((bool) null);
var_dump($c->getSlaveOkay());
?>
--EXPECTF--
Deprecated: Function MongoCollection::setSlaveOkay() is deprecated in %smongocollection-getslaveokay.php on line %d

Deprecated: Function MongoCollection::getSlaveOkay() is deprecated in %smongocollection-getslaveokay.php on line %d
bool(true)

Deprecated: Function MongoCollection::setSlaveOkay() is deprecated in %smongocollection-getslaveokay.php on line %d

Deprecated: Function MongoCollection::getSlaveOkay() is deprecated in %smongocollection-getslaveokay.php on line %d
bool(false)
