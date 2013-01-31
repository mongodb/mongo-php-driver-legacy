--TEST--
MongoGridFS::findOne() with criteria argument
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
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
