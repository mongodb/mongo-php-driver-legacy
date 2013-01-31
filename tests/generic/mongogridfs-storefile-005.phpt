--TEST--
MongoGridFS::storeFile() with empty file
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

$gridfs->storeFile('tests/empty');

$file = $gridfs->findOne();

var_dump(0 === $file->file['length']);
--EXPECT--
bool(true)
