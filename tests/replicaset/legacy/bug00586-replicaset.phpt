--TEST--
Test for PHP-586: GridFS should only do one GLE
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();
$gridfs->storeFile(__FILE__, array('x' => 1), array("w" => 1));

$file = $gridfs->findOne(array(), array('x' => 1));

try {
    $file->getBytes();
    var_dump(false);
} catch (MongoGridFSException $e) {
    var_dump(true);
}

try {
    /* FIXME: The timeout is broken and seems hardcoded. The actual timeout is irrelevant to this test, as long as it timesout the replication at all :) */
    $gridfs->storeFile(__FILE__, array('x' => 1), array("w" => 42, "wTimeoutMS" => 42));
} catch(MongoGridFSException $e) {
    var_dump($e->getMessage(), $e->getCode());
}
?>
--EXPECTF--
bool(true)
string(%d) "Could not store file: %s:%d:%stime%S"
int(%d)
