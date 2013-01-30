--TEST--
MongoGridFS::storeBytes() with metadata argument
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

$gridfs->storeBytes('foobar', array('x' => 1));

$file = $gridfs->findOne();

var_dump(1 === $file->file['x']);
--EXPECT--
bool(true)
