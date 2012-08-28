--TEST--
MongoGridFSFile::getBytes() returns all file data from all chunks
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
