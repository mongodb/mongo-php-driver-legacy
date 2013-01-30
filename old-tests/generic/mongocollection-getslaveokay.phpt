--TEST--
MongoCollection::getSlaveOkay()
--DESCRIPTION--
Test both true and false for slave_okay attribute
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();

$c = $m->phpunit->col;
$c->setSlaveOkay((bool) array('foo'));
var_dump($c->getSlaveOkay());
$c->setSlaveOkay((bool) null);
var_dump($c->getSlaveOkay());
?>
--EXPECTF--
%s: Function MongoCollection::setSlaveOkay() is deprecated in %s on line %d

%s: Function MongoCollection::getSlaveOkay() is deprecated in %s on line %d
bool(true)

%s: Function MongoCollection::setSlaveOkay() is deprecated in %s on line %d

%s: Function MongoCollection::getSlaveOkay() is deprecated in %s on line %d
bool(false)

