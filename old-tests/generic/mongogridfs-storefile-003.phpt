--TEST--
MongoGridFS::storeFile() with large file
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

$gridfs->storeFile('tests/Formelsamling.pdf');

$file = $gridfs->findOne();

var_dump(file_get_contents('tests/Formelsamling.pdf') === $file->getBytes());
--EXPECT--
bool(true)
