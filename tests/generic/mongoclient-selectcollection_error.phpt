--TEST--
MongoClient::selectCollection
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (!version_compare(phpversion(), "5.5", '>=')) echo "skip >= PHP 5.5 needed\n"; ?>
--INI--
mongo.long_as_object=0
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mongo = new \MongoClient($host);
try {
	$collection = $mongo->selectCollection('', '');
} catch (MongoException $e) {
	echo $e->getMessage(), "\n";
	echo $e->getCode(), "\n";
}
?>
--EXPECT--
MongoDB::__construct(): invalid name 
2
