--TEST--
MongoGridFSCursor::current()
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
var_dump(null === $cursor->current());

$next = $cursor->getNext();

var_dump($next instanceof MongoGridFSFile);
var_dump($next == $cursor->current());
var_dump($next !== $cursor->current());
var_dump(null === $cursor->getNext());
var_dump(null === $cursor->current());
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
