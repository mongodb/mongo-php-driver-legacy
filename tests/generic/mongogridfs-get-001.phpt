--TEST--
MongoGridFS::get()
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

$file = $gridfs->get($id);

var_dump($id == $file->file['_id']);
var_dump(__FILE__ === $file->file['filename']);
--EXPECT--
bool(true)
bool(true)
