--TEST--
MongoGridFS::remove() with safe option
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();
$id = $gridfs->storeFile(__FILE__);

$result = $gridfs->remove(array('_id' => $id), array('w' => true));
var_dump((bool) $result['ok']);
var_dump(1 === $result['n']);
var_dump(null === $result['err']);
--EXPECT--
bool(true)
bool(true)
bool(true)
