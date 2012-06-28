--TEST--
MongoCollection::getSlaveOkay()
--DESCRIPTION--
Test both true and false for slave_okay attribute
--FILE--
<?php
$m = new Mongo();
$c = $m->phpunit->col;
$c->setSlaveOkay((bool) array('foo'));
var_dump($c->getSlaveOkay());
$c->setSlaveOkay((bool) null);
var_dump($c->getSlaveOkay());
?>
--EXPECT--
bool(true)
bool(false)
