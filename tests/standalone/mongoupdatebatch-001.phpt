--TEST--
MongoUpdateBatch: Adding documents to Update Batch
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
$collection->insert($insertdoc1);
$insertdoc1 = array("my" => "demo");
$collection->insert($insertdoc1);
$insertdoc1 = array("my" => "demo");
$collection->insert($insertdoc1);

$update1 = array("q" => array("_id" => $insertdoc1["_id"]), "u" => array('$set' => array("c" => 2)));
$batch = new MongoUpdateBatch($collection);

$addretval = $batch->add($update1);
$exeretval = $batch->execute(array("w" => 1));
var_dump($addretval, $exeretval[0]["ok"], $exeretval[0]["n"]);


echo "Now with multi=true\n";
$update1 = array("q" => array("my" => "demo"), "u" => array('$set' => array("d" => 2)), "multi" => true);
$batch = new MongoUpdateBatch($collection);

$addretval = $batch->add($update1);
$exeretval = $batch->execute(array("w" => 1));
var_dump($addretval, $exeretval[0]["ok"], $exeretval[0]["n"]);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
bool(true)
bool(true)
int(1)
Now with multi=true
bool(true)
bool(true)
int(3)
===DONE===
