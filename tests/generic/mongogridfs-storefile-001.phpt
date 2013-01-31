--TEST--
MongoGridFS::storeFile() returns a generated ID
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();
$id = $gridfs->storeFile(__FILE__);

var_dump($id instanceof MongoId);

$file = $gridfs->findOne();
var_dump($id == $file->file['_id']);
--EXPECT--
bool(true)
bool(true)
