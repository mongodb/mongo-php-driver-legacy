--TEST--
Test for PHP-1156: Negative values for timeouts should block indefinitely (streams)
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (getenv('SKIP_SLOW_TESTS')) die('skip slow tests excluded by request'); ?>
--INI--
default_socket_timeout=1
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::CON, MongoLog::FINE, "/timeout/");

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('connectTimeoutMS' => 0, 'socketTimeoutMS' => 0));
echo "Connected\n";

echo "\nDropping collection\n";
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

echo "\nInserting one document\n";
$collection->insert(array('x' => 1));

echo "\nExecuting find() with 4-second sleep and no timeout\n";
$cursor = $collection->find(array('$where' => 'sleep(2000) || true'));
$cursor->timeout(-1);
iterator_to_array($cursor);

// Issue the query that times out last to avoid extra reconnection log messages
echo "\nExecuting find() with 4-second sleep and default timeout\n";
$cursor = $collection->find(array('$where' => 'sleep(2000) || true'));

try {
    iterator_to_array($cursor);
    echo "FAILED\n";
} catch (MongoCursorTimeoutException $e) {
    echo "find() timed out as expected\n";
}

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
Initializing cursor timeout to 0 (from connection options)
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d

Inserting one document
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d

Executing find() with 4-second sleep and no timeout
Initializing cursor timeout to 0 (from connection options)
Setting the stream timeout to -1.000000
Stream timeout will be reverted to default_socket_timeout (1)
Now setting stream timeout back to 1.000000
Setting the stream timeout to -1.000000
Stream timeout will be reverted to default_socket_timeout (1)
Now setting stream timeout back to 1.000000

Executing find() with 4-second sleep and default timeout
Initializing cursor timeout to 0 (from connection options)
No timeout changes for %s:%d;-;.;%d
find() timed out as expected
===DONE===
