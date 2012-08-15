--TEST--
MongoGridFS::findOne() with selected fields
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();
$gridfs->storeFile(__FILE__, array('x' => 1, 'y' => 'foo', 'z' => 'bar'));

$file = $gridfs->findOne(array(), array('x' => 1, 'y' => 1));

var_dump(1 === $file->file['x']);
var_dump('foo' === $file->file['y']);
var_dump(array_key_exists('z', $file->file));
--EXPECT--
bool(true)
bool(true)
bool(false)
