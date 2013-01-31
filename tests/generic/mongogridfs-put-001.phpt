--TEST--
MongoGridFS::put()
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();
$id = $gridfs->put(__FILE__);

$file = $gridfs->get($id);

var_dump($id == $file->file['_id']);
var_dump(__FILE__ === $file->file['filename']);
--EXPECT--
bool(true)
bool(true)
