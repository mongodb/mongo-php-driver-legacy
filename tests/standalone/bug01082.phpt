--TEST--
Test for PHP-1082: CommandCursor segfaults when server is closed
--SKIPIF--
<?php $needs = "2.6.0"; $needsOp = "gt"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

foreach (range(1,5000) as $x) {
    $collection->insert(array('x' => $x));
}

$cursors = $collection->parallelCollectionScan(3);

$mc->close();
foreach($cursors as $cursor) {
    var_dump($cursor);

    try {
        foreach($cursor as $entry) {
        }
        var_dump($entry);
    } catch(MongoConnectionException $e) {
        echo $e->getMessage(), "\n";
        break;
    }
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
object(MongoCommandCursor)#%d (0) {
}
the connection has been terminated, and this cursor is dead
===DONE===

