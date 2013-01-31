--TEST--
MongoGridFSFile::getFilename()
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
var_dump(__FILE__ === $file->getFilename());
var_dump($file->file['filename'] === $file->getFilename());
--EXPECT--
bool(true)
bool(true)
