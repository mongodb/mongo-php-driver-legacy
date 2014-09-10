--TEST--
Test for PHP-1099: socketTimeoutMS=-1 doesn't work (negative socketTimeoutMS)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::CON, MongoLog::FINE, "/timeout/");

// This should have no effect on socketTimeoutMS
MongoCursor::$timeout = -2;

$host = MongoShellServer::getStandaloneInfo();
$dsn = "mongodb://$host/?socketTimeoutMS=-1&connectTimeoutMS=-1";
$mc = new MongoClient($dsn);
echo "Connected\n";

$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();
echo "Dropped\n";

$cursor = $collection->findOne();
echo "findOne done\n";

echo "\n\nTimeout 20\n";
$cursor = $collection->find();
$cursor->timeout(20);
iterator_to_array($cursor);

echo "\n\nTimeout 42\n";
MongoCursor::$timeout = 42;
$cursor = $collection->find();
iterator_to_array($cursor);

echo "\n\nTimeout -1\n";
$cursor = $collection->find();
$cursor->timeout(-1);
iterator_to_array($cursor);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Connecting to tcp://%s:%d (%s:%d;-;.;%d) with connection timeout: -1.000000
Setting stream timeout to -1.000000
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
Connected
Initializing cursor timeout to -1 (from connection options)
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
Dropped
Initializing cursor timeout to -1 (from connection options)
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
findOne done


Timeout 20
Initializing cursor timeout to -1 (from connection options)
Setting the stream timeout to 0.020000
Now setting stream timeout back to -1.000000
Setting the stream timeout to 0.020000
Now setting stream timeout back to -1.000000


Timeout 42

%s: The 'MongoCursor::$timeout' static property is deprecated, please call MongoCursor->timeout() instead in %s on line %d
Initializing cursor timeout to 42 (from deprecated static property)
Setting the stream timeout to 0.042000
Now setting stream timeout back to -1.000000
Setting the stream timeout to 0.042000
Now setting stream timeout back to -1.000000


Timeout -1

%s: The 'MongoCursor::$timeout' static property is deprecated, please call MongoCursor->timeout() instead in %s on line %d
Initializing cursor timeout to 42 (from deprecated static property)
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
===DONE===
