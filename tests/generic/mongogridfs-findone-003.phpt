--TEST--
MongoGridFS::findOne() with criteria argument
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

var_dump(null === $gridfs->findOne(array('filename' => __FILE__)));

$gridfs->storeFile(__FILE__);

var_dump($gridfs->findOne(array('filename' => __FILE__)) instanceof MongoGridFSFile);
var_dump(null === $gridfs->findOne(array('filename' => '/does/not/exist')));
--EXPECT--
bool(true)
bool(true)
bool(true)
