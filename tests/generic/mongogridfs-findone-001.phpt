--TEST--
MongoGridFS::findOne() with no arguments
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

var_dump(null === $gridfs->findOne());

$gridfs->storeFile(__FILE__);

var_dump($gridfs->findOne() instanceof MongoGridFSFile);
--EXPECT--
bool(true)
bool(true)
