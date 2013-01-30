--TEST--
MongoGridFSFile::getFilename()
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
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
