--TEST--
Test for PHP-375: GridFS segfaults when there are more chunks than expected
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = new_mongo_standalone();
$gridfs = $m->selectDb(dbname())->getGridfs();
$gridfs->remove();

$id = $gridfs->put(__FILE__);
$coll = $m->selectDB(dbname())->selectCollection("fs.chunks");

// Fetch the first (only) chunk
$chunk = $coll->find(array("files_id" => $id))->getNext();
// Unset the id and bump the chunk# to create unexpectedly many chunks
unset($chunk["_id"]);
$chunk["n"]++;
$coll->insert($chunk);


// Now fetch the inserted file
$file = $gridfs->findOne(array("_id" => $id));

// Throws exception about to many chunks
try {
    $file->getBytes();
} catch(MongoGridFSException $e) {
    var_dump($e->getMessage(), $e->getCode());
}
?>
--EXPECTF--
string(%d) "There is more data associated with this file than the metadata specifies (reading chunk 1)"
int(1)

