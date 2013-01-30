--TEST--
MongoGridFS::delete()
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

var_dump(0 < $gridfs->chunks->count());
var_dump(1 === $gridfs->count());

$gridfs->delete($id);

var_dump(0 === $gridfs->chunks->count());
var_dump(0 === $gridfs->count());
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
