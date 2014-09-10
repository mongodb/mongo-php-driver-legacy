--TEST--
Test for PHP-1155: Don't swap timeout in php_mongo_io_stream_read() unnecessarily
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::CON, MongoLog::FINE, "/timeout/");

/* Since connectTimeoutMS defaults to 60000 (60 seconds) and socketTimeoutMS is
 * applied as the default stream timeout, we should expect timeout swaps for
 * each operation during the connection step.
 */
$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('socketTimeoutMS' => -1));
echo "Connected\n";

echo "\nDropping collection\n";
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

echo "\nExecuting find() with default timeout\n";
$cursor = $collection->find();
iterator_to_array($cursor);

/* Negative values block indefinitely and should be considered equivalent, so do
 * not expect any timeout swaps for the following queries.
 */
echo "\nExecuting find() with MongoCursor::timeout(): -2\n";
$cursor = $collection->find();
$cursor->timeout(-2);
iterator_to_array($cursor);

/* Using -3 here, as -2 happens to be the static property's intializer value,
 * which we use for change tracking.
 */
echo "\nExecuting find() with MongoCursor::\$timeout: -3\n";
MongoCursor::$timeout = -3;
$cursor = $collection->find();
iterator_to_array($cursor);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Connecting to tcp://%s:%d (%s:%d;-;.;%d) with connection timeout: 60.000000
Setting stream timeout to -1.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to -1.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to -1.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to -1.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to -1.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to -1.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to -1.000000
Connected

Dropping collection
Initializing cursor timeout to -1 (from connection options)
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d

Executing find() with default timeout
Initializing cursor timeout to -1 (from connection options)
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d

Executing find() with MongoCursor::timeout(): -2
Initializing cursor timeout to -1 (from connection options)
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d

Executing find() with MongoCursor::$timeout: -3

%s: The 'MongoCursor::$timeout' static property is deprecated, please call MongoCursor->timeout() instead in %s on line %d
Initializing cursor timeout to -3 (from deprecated static property)
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
===DONE===
