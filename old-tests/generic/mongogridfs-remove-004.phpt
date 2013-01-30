--TEST--
MongoGridFS::remove() with safe option
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();
$id = $gridfs->storeFile(__FILE__);

$result = $gridfs->remove(array('_id' => $id), array('safe' => true));
var_dump((bool) $result['ok']);
var_dump(1 === $result['n']);
var_dump(null === $result['err']);
--EXPECT--
bool(true)
bool(true)
bool(true)
