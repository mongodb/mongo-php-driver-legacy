--TEST--
Database: toString.
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$a = mongo();
$d = $a->selectDb("phpunit");
echo $d, "\n";
echo $d->__toString(), "\n";
?>
--EXPECT--
phpunit
phpunit
