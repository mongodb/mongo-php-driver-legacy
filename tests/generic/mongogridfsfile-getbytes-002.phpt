--TEST--
MongoGridFSFile::getBytes() returns all file data from all chunks
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

$chunks = $db->selectCollection('fs.chunks')->find(array('files_id' => $id));

$chunksData = '';

foreach ($chunks as $chunk) {
    $chunksData .= $chunk['data']->bin;
}

$contents = file_get_contents(__FILE__);

var_dump($contents === $gridfs->findOne()->getBytes());
var_dump($contents === $chunksData);
--EXPECT--
bool(true)
bool(true)
