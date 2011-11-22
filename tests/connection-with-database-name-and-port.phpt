--TEST--
Connection strings: with database name and port
--FILE--
<?php
new Mongo("localhost:27017/phpunit");
new Mongo("localhost:27017/bar/baz");
new Mongo("mongodb://localhost:27017/");
new Mongo("localhost:27017/");
new Mongo("localhost:27017,localhost:27019/");
?>
--EXPECTF--
