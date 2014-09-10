--TEST--
Test for PHP-1155: Revert to default_socket_timeout if socketTimeoutMS is zero (default connectTimeoutMS)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::CON, MongoLog::FINE, "/timeout/");

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('socketTimeoutMS' => 0));
echo "Connected\n";

echo "\nDropping collection\n";
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

echo "\nExecuting find() with default timeout\n";
$cursor = $collection->find();
iterator_to_array($cursor);

echo "\nExecuting find() with MongoCursor::timeout(): 1000\n";
$cursor = $collection->find();
$cursor->timeout(1000);
iterator_to_array($cursor);

echo "\nExecuting find() with MongoCursor::\$timeout: 1000\n";
MongoCursor::$timeout = 1000;
$cursor = $collection->find();
iterator_to_array($cursor);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Connecting to tcp://%s:%d (%s:%d;-;.;%d) with connection timeout: 60.000000
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
Connected

Dropping collection
Initializing cursor timeout to 0 (from connection options)
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d

Executing find() with default timeout
Initializing cursor timeout to 0 (from connection options)
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d

Executing find() with MongoCursor::timeout(): 1000
Initializing cursor timeout to 0 (from connection options)
Setting the stream timeout to 1.000000
Stream timeout will be reverted to default_socket_timeout (60)
Now setting stream timeout back to 60.000000
Setting the stream timeout to 1.000000
Stream timeout will be reverted to default_socket_timeout (60)
Now setting stream timeout back to 60.000000

Executing find() with MongoCursor::$timeout: 1000

%s: The 'MongoCursor::$timeout' static property is deprecated, please call MongoCursor->timeout() instead in %s on line %d
Initializing cursor timeout to 1000 (from deprecated static property)
Setting the stream timeout to 1.000000
Stream timeout will be reverted to default_socket_timeout (60)
Now setting stream timeout back to 60.000000
Setting the stream timeout to 1.000000
Stream timeout will be reverted to default_socket_timeout (60)
Now setting stream timeout back to 60.000000
===DONE===
