--TEST--
MongoInsertBatch: Execute the same batch twice
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);

$collection = $mc->selectCollection("test", "insertbatch");
$collection->drop();


$insertdoc1 = array("my" => "demo");

$batch = new MongoInsertBatch($collection);
$addretval = $batch->add($insertdoc1);
$exeretval = $batch->execute(array("w" => 1));

try {
    $exeretval = $batch->execute(array("w" => 1));
    echo "FAILED - That should have thrown an exception\n";
} catch(MongoException $e) {
    var_dump(get_class($e), $e->getMessage());
}
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
string(14) "MongoException"
string(17) "No items in batch"
===DONE===

