--TEST--
MongoGridFS::findOne() with no arguments
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

var_dump(null === $gridfs->findOne());

$gridfs->storeFile(__FILE__);

var_dump($gridfs->findOne() instanceof MongoGridFSFile);
--EXPECT--
bool(true)
bool(true)
