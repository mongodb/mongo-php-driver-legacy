--TEST--
MongoGridFS::storeFile() throws exception if insert fails with safe option
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();
$gridfs->storeFile(__FILE__, array('_id' => 1), array('w' => true));

try {
    $gridfs->storeFile(__FILE__, array('_id' => 1), array('w' => true));
    var_dump(false);
} catch (MongoGridFSException $e) {
    var_dump($e->getMessage(), $e->getCode());
}
--EXPECTF--
string(%d) "Could not store file:%sE11000 duplicate key error index: %s.fs.chunks.$files_id_1_n_1  dup key: { : 1, : 0 }"
int(11000)
