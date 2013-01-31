--TEST--
MongoGridFS::storeFile() returns a custom ID
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();
$id = $gridfs->storeFile(__FILE__, array('_id' => 1));

var_dump(1 === $id);

$file = $gridfs->findOne();
var_dump($id == $file->file['_id']);
--EXPECT--
bool(true)
bool(true)
