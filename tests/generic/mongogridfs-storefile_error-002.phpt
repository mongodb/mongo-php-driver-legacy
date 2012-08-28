--TEST--
MongoGridFS::storeFile() throws exception when overwriting files
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

$gridfs->storeFile(__FILE__, array('_id' => 1));

try {
    $gridfs->storeFile(__FILE__, array('_id' => 1));
} catch (MongoGridFSException $e) {
    echo $e->getMessage();
}
--EXPECTF--
Could not store file: E11000 duplicate key error index: %s.fs.chunks.$files_id_1_n_1  dup key: { : 1, : 0 }
