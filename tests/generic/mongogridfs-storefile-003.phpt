--TEST--
MongoGridFS::storeFile() with large file
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

$gridfs->storeFile('tests/data-files/mongogridfs-storefile-003.pdf');

$file = $gridfs->findOne();

var_dump(file_get_contents('tests/data-files/mongogridfs-storefile-003.pdf') === $file->getBytes());
--EXPECT--
bool(true)
