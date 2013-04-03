--TEST--
MongoGridFS::storeFile() throws exception when overwriting files
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

$gridfs->storeFile(__FILE__, array('_id' => 1));

try {
    $gridfs->storeFile(__FILE__, array('_id' => 1));
} catch (MongoGridFSException $e) {
    echo $e->getMessage(), "\n";
}
--EXPECTF--
Could not store file:%sE11000 duplicate key error index: %s.fs.chunks.$files_id_1_n_1  dup key: { : 1, : 0 }
