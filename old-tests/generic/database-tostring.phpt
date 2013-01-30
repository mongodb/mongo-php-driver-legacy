--TEST--
Database: toString.
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$a = mongo();
$d = $a->selectDb("phpunit");
echo $d, "\n";
echo $d->__toString(), "\n";
?>
--EXPECT--
phpunit
phpunit
