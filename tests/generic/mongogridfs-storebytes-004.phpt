--TEST--
MongoGridFS::storeBytes() with metadata argument
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

$gridfs->storeBytes('foobar', array('x' => 1));

$file = $gridfs->findOne();

var_dump(1 === $file->file['x']);
--EXPECT--
bool(true)
