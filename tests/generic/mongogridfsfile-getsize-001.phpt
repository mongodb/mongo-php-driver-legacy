--TEST--
MongoGridFSFile::getSize()
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
var_dump(filesize(__FILE__) === $file->getSize());
var_dump($file->file['length'] === $file->getSize());
--EXPECT--
bool(true)
bool(true)
