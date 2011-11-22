--TEST--
Connection strings: Prefixed with mongodb://
--FILE--
<?php
$a = new Mongo("mongodb://localhost", false);
var_dump($a->connected);
$a = new Mongo("mongodb://localhost");
var_dump($a->connected);
$b = new Mongo("mongodb://localhost:27017", false);
var_dump($b->connected);
$b = new Mongo("mongodb://localhost:27017");
var_dump($b->connected);
--EXPECT--
bool(false)
bool(true)
bool(false)
bool(true)
