--TEST--
Test for bug PHP-300: execute crashes with array() as argument
--FILE--
<?php
$m = new Mongo;
$db = $m->phpunit;
$db->execute(array());
?>
--EXPECTF--
Fatal error: MongoDB::execute(): The argument is neither an object of MongoCode or a string in %s on line 4
