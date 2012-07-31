--TEST--
MongoCollection::getSlaveOkay()
--DESCRIPTION--
Test both true and false for slave_okay attribute
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
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
