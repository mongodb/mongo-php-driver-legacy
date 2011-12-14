--TEST--
Connection strings: Test single host name with/without port
--FILE--
<?php
$a = new Mongo("localhost");
var_dump($a->connected);
$b = new Mongo("localhost:27017");
var_dump($b->connected);
--EXPECT--
bool(true)
bool(true)
