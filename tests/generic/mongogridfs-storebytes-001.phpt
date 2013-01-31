--TEST--
MongoGridFS::storeBytes() returns a generated ID
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();
$id = $gridfs->storeBytes('foobar');

var_dump($id instanceof MongoId);

$file = $gridfs->findOne();
var_dump($id == $file->file['_id']);
--EXPECT--
bool(true)
bool(true)
