--TEST--
MongoInsertBatch: Order true/false failures
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php if ( ! class_exists('MongoWriteBatch')) { exit('skip This test requires MongoWriteBatch classes'); } ?>
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
$retval = $batch->add(array("_id" => "duplicatedid", "document" => "doc#1"));
$retval = $batch->add(array("_id" => "duplicatedid", "document" => "doc#2"));
for ($i=1; $i<=2999; $i++) {
    $retval = $batch->add(array("document" => $i));
}

try {
    $exeretval = $batch->execute(array("w" => 1));
    echo "FAILED - That should have thrown an exception\n";
} catch(MongoException $e) {
    var_dump(get_class($e), $e->getMessage());
}
var_dump($collection->find()->count());
$collection->drop();



echo "Ordered=false should continue inserting\n";
$batch = new MongoInsertBatch($collection, array("ordered" => false));
$retval = $batch->add(array("_id" => "duplicatedid", "document" => "doc#1"));
$retval = $batch->add(array("_id" => "duplicatedid", "document" => "doc#2"));
for ($i=1; $i<=2999; $i++) {
    $retval = $batch->add(array("document" => $i));
}

try {
    $exeretval = $batch->execute(array("w" => 1));
    echo "FAILED - That should have thrown an exception\n";
} catch(MongoException $e) {
    var_dump(get_class($e), $e->getMessage());
}
var_dump($collection->find()->count());
$collection->drop();




echo "Ordered=true should stop inserting\n";
$batch = new MongoInsertBatch($collection, array("ordered" => false));
for ($i=1; $i<=1001; $i++) {
    $retval = $batch->add(array("document" => $i));
}
$retval = $batch->add(array("_id" => "duplicatedid", "document" => "doc#1"));
$retval = $batch->add(array("_id" => "duplicatedid", "document" => "doc#2"));
for ($i=1; $i<=1001; $i++) {
    $retval = $batch->add(array("document" => $i));
}

try {
    $exeretval = $batch->execute(array("w" => 1));
    echo "FAILED - That should have thrown an exception\n";
} catch(MongoException $e) {
    var_dump(get_class($e), $e->getMessage());
}
var_dump($collection->find()->count());
$collection->drop();
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
string(26) "MongoDuplicateKeyException"
string(141) "127.0.0.1:30000: insertDocument :: caused by :: 11000 E11000 duplicate key error index: test.insertbatch.$_id_  dup key: { : "duplicatedid" }"
int(1)
Ordered=false should continue inserting
string(26) "MongoDuplicateKeyException"
string(141) "127.0.0.1:30000: insertDocument :: caused by :: 11000 E11000 duplicate key error index: test.insertbatch.$_id_  dup key: { : "duplicatedid" }"
int(3000)
Ordered=true should stop inserting
string(26) "MongoDuplicateKeyException"
string(141) "127.0.0.1:30000: insertDocument :: caused by :: 11000 E11000 duplicate key error index: test.insertbatch.$_id_  dup key: { : "duplicatedid" }"
int(2003)
===DONE===
