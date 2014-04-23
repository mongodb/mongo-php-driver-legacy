--TEST--
MongoInsertBatch->execute(): Optional Write Concern
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php if ( ! class_exists('MongoWriteBatch')) { exit('skip This test requires MongoWriteBatch classes'); } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();


$insertdoc1 = array("my" => "demo");
$insertdoc2 = array("is" => "working");
$insertdoc3 = array("pretty" => "well");

$batch = new MongoInsertBatch($collection);
$addretval = $batch->add($insertdoc1);
$addretval = $batch->add($insertdoc2);
$addretval = $batch->add($insertdoc3);
$exeretval = $batch->execute();

var_dump($exeretval["ok"], $exeretval["nInserted"]);
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
bool(true)
int(3)
===DONE===
