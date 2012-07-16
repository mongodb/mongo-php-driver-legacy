--TEST--
Test for bug PHP-359: getPID() does not return expected PID when called on custom MongoID object
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php


$id = new MongoId;
var_dump($id->getPid(), $id->__toString(), $id->getPid() == getmypid());

$id = new MongoID("4fe3420a44415ecc83000000");
var_dump($id->getPid(), $id->__toString());

$id = new MongoID("4fe3427744415e4f84000001");
var_dump($id->getPid(), $id->__toString());

$id = new MongoID("4fe342a944415e5284000000");
var_dump($id->getPid(), $id->__toString());
?>
--EXPECTF--
int(%d)
string(24) "%s"
bool(true)
int(33740)
string(24) "4fe3420a44415ecc83000000"
int(33871)
string(24) "4fe3427744415e4f84000001"
int(33874)
string(24) "4fe342a944415e5284000000"

