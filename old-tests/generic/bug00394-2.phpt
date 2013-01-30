--TEST--
Test for PHP-394: Crashes and mem leaks. (2)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
$x = new MongoId(NULL);
$x->__construct(NULL);
$x->getInc(NULL);
var_dump($x->__toString());
?>
===DONE===
--EXPECTF--
string(24) "%s"
===DONE===

