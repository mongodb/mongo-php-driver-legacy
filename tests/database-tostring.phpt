--TEST--
Database: toString.
--FILE--
<?php
$a = new Mongo("localhost");
$d = $a->selectDb("phpunit");
echo $d, "\n";
echo $d->__toString(), "\n";
?>
--EXPECT--
phpunit
phpunit
