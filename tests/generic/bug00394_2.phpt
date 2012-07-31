--TEST--
Test for bug PHP-394 (2): Crashes and mem leaks
--SKIPIF--
<?php require_once __DIR__ . "/skipif.inc" ?>
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

