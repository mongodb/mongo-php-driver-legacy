--TEST--
MongoGridFSCursor::getNext()
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

$cursor = $gridfs->find();

var_dump($cursor instanceof MongoGridFSCursor);
var_dump($cursor->getNext() instanceof MongoGridFSFile);
var_dump(null === $cursor->getNext());
--EXPECT--
bool(true)
bool(true)
bool(true)
