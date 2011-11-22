--TEST--
Connection strings: with database name
--FILE--
<?php
new Mongo("mongodb://localhost/phpunit");
new Mongo("mongodb://localhost/bar/baz");
new Mongo("mongodb://localhost/");
?>
--EXPECTF--
