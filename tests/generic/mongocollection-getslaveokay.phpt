--TEST--
MongoCollection::getSlaveOkay()
--DESCRIPTION--
Test both true and false for slave_okay attribute
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$m = mongo();

$c = $m->phpunit->col;
$c->setSlaveOkay((bool) array('foo'));
var_dump($c->getSlaveOkay());
$c->setSlaveOkay((bool) null);
var_dump($c->getSlaveOkay());
?>
--EXPECT--
bool(true)
bool(false)
