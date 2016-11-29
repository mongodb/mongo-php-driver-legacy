--TEST--
Test for PHP-1446: GridFS MD5 fails with hashed "files_id" shard key
--SKIPIF--
<?php require_once "tests/utils/mongos.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$cfg = MongoShellServer::getShardInfo();
$mc = new MongoClient($cfg[0]);

$db = $mc->selectDB(dbname());
$gridfs = $db->getGridFS(collname(__FILE__));
$gridfs->drop();

// Don't check the result, since the database may already have sharding enabled
$mc->selectDB('admin')->command(array('enableSharding' => dbname()));

$result = $mc->selectDB('admin')->command(array(
    'shardCollection' => (string) $gridfs->chunks,
    'key' => array('files_id' => 'hashed'),
));

var_dump(!empty($result['ok']));

$id = $gridfs->storeFile(__FILE__);
$file = $gridfs->get($id);

var_dump(isset($file->file['md5']));

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
bool(true)
bool(true)
===DONE===
