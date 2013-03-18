--TEST--
MongoGridFSFile::getBytes()
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

$gridfs->storeFile(__FILE__);

$file = $gridfs->findOne();

var_dump(file_get_contents(__FILE__) === $file->getBytes());
--EXPECT--
bool(true)
