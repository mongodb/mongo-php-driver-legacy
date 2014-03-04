--TEST--
MongoInsertBatch: Basic add/execute
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
$insertdoc2 = array("is" => "working");
$insertdoc3 = array("pretty" => "well");

$batch = new MongoInsertBatch($collection);
$addretval = $batch->add($insertdoc1);
var_dump($addretval);
$addretval = $batch->add($insertdoc2);
var_dump($addretval);
$addretval = $batch->add($insertdoc3);
var_dump($addretval);
$exeretval = $batch->execute(array("w" => 1));

var_dump($exeretval["ok"], $exeretval["nInserted"]);
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
bool(true)
bool(true)
bool(true)
bool(true)
int(3)
===DONE===
