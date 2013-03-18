--TEST--
MongoGridFSFile::getSize()
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
var_dump(filesize(__FILE__) === $file->getSize());
var_dump($file->file['length'] === $file->getSize());
--EXPECT--
bool(true)
bool(true)
