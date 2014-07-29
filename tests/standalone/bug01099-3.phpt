--TEST--
Test for PHP-1099: socketTimeoutMS=-1 doesn't work (default socketTimeoutMS)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::CON, MongoLog::FINE, "/timeout/");

// This should have no effect on socketTimeoutMS
MongoCursor::$timeout = -2;

$host = MongoShellServer::getStandaloneInfo();
$dsn = "mongodb://$host/";
$mc = new MongoClient($dsn);
echo "Connected\n";
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

$cursor = $collection->findOne();
echo "findOne done\n";
$cursor = $collection->find();

echo "\n\nTimeout 20\n";
$cursor->timeout(20);
iterator_to_array($cursor);

echo "\n\nTimeout 43\n";
MongoCursor::$timeout = 43;
$cursor = $collection->find();
iterator_to_array($cursor);

echo "\n\nTimeout -1\n";
$cursor = $collection->find();
$cursor->timeout(-1);
iterator_to_array($cursor);

echo "\n\nTimeout 42\n";
$cursor = $collection->find();
$cursor->timeout(42);
iterator_to_array($cursor);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Connecting to tcp://%s:%d (%s:%d;-;.;%d) with connection timeout: 60.000000
Setting stream timeout to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Connected
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
findOne done


Timeout 20
Setting the stream timeout to 0.020000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 0.020000
Now setting stream timeout back to 30.000000


Timeout 43

%s: The 'MongoCursor::$timeout' static property is deprecated, please call MongoCursor->timeout() instead in %s on line %d
Setting the stream timeout to 0.043000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 0.043000
Now setting stream timeout back to 30.000000


Timeout -1

%s: The 'MongoCursor::$timeout' static property is deprecated, please call MongoCursor->timeout() instead in %s on line %d
Setting the stream timeout to -1.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to -1.000000
Now setting stream timeout back to 30.000000


Timeout 42

%s: The 'MongoCursor::$timeout' static property is deprecated, please call MongoCursor->timeout() instead in %s on line %d
Setting the stream timeout to 0.042000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 0.042000
Now setting stream timeout back to 30.000000
===DONE===
