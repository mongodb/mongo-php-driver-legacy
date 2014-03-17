--TEST--
Test for PHP-778: Deprecate static properties
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('w' => 'majority'));

$collection = $mc->selectCollection(dbname(), collname(__FILE__));

$cursor = $collection->find();
echo "No deprecation notice\n";

echo "Now we'll trigger a timeout notice\n";
MongoCursor::$timeout = 42;
$cursor = $collection->find();



echo "Now we'll trigger a slaveOkay (and timeout) notice\n";
MongoCursor::$slaveOkay = false;
$collection->find();

?>
===DONE===
--EXPECTF--
No deprecation notice
Now we'll trigger a timeout notice

%s: MongoCollection::find(): The 'MongoCursor::$timeout' static property is deprecated, please call MongoCursor->timeout() instead in %s on line %d
Now we'll trigger a slaveOkay (and timeout) notice

%s: MongoCollection::find(): The 'MongoCursor::$timeout' static property is deprecated, please call MongoCursor->timeout() instead in %s on line %d

%s: MongoCollection::find(): The 'slaveOkay' option is deprecated. Please switch to read-preferences in %s on line %d
===DONE===

