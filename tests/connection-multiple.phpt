--TEST--
Connection strings: Test multiple host names with/without port
--FILE--
<?php
$a = new Mongo("localhost,127.0.0.2");
var_dump($a->connected);
$b = new Mongo("localhost:27017,127.0.0.2:27017");
var_dump($b->connected);
--EXPECT--
bool(true)
bool(true)
