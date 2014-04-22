--TEST--
Test for PHP-1080: MongoCursor::$timeout=-1 doesn't work anymore
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array("socketTimeoutMS" => -1));
$db = $mc->selectDb(dbname());

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

MongoCursor::$timeout = -1;
$collection->findOne();
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
%s: MongoCollection::findOne(): The 'MongoCursor::$timeout' static property is deprecated, please call MongoCursor->timeout() instead in %s on line %d
===DONE===

