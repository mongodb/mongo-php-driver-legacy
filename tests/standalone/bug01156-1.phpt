--TEST--
Test for PHP-1156: Zero values for timeouts should use default_socket_timeout (streams)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
--INI--
default_socket_timeout=20
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::CON, MongoLog::FINE, "/timeout/");

/* The absence of timeout swaps for connection operations is evidence that
 * default_socket_timeout is used when connectTimeoutMS is zero.
 */
$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('connectTimeoutMS' => 0, 'socketTimeoutMS' => 0));
echo "Connected\n";

echo "\nDropping collection\n";
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

echo "\nExecuting find() with default timeout\n";
$cursor = $collection->find();
iterator_to_array($cursor);

// Timeout swaps will infer that default_socket_timeout is used by default
echo "\nExecuting find() with MongoCursor::timeout(): 4321\n";
$cursor = $collection->find();
$cursor->timeout(4321);
iterator_to_array($cursor);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Connecting to tcp://%s:%d (%s:%d;-;.;%d) without connection timeout (default_socket_timeout will be used)
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
Connected

Dropping collection
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d

Executing find() with default timeout
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d

Executing find() with MongoCursor::timeout(): 4321
Setting the stream timeout to 4.321000
Stream timeout will be reverted to default_socket_timeout (20)
Now setting stream timeout back to 20.000000
Setting the stream timeout to 4.321000
Stream timeout will be reverted to default_socket_timeout (20)
Now setting stream timeout back to 20.000000
===DONE===
