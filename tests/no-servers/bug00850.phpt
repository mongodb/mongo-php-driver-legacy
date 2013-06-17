--TEST--
Test for PHP-850: Conditional jump on empty server name to MongoClient
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
$mc = new MongoClient("", array("connect" => false));
var_dump($mc->__toString());
?>
--EXPECTF--
string(15) "localhost:27017"
