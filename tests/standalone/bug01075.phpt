--TEST--
Test for PHP-1075: close() with parallelCollectionScan() segfaults upon request shutdown
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
echo "The command cursor should not segfault during shutdown now! :)\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
The command cursor should not segfault during shutdown now! :)
===DONE===

