--TEST--
Test for PHP-300: execute crashes with array() as argument.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();
$db = $m->phpunit;
$db->execute(array());
?>
--EXPECTF--
Deprecated: Function MongoDB::execute() is deprecated in %s on line %d

Fatal error: MongoDB::execute(): The argument is neither an object of MongoCode or a string in %s on line %d
