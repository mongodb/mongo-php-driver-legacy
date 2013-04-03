--TEST--
MongoGridFS::put() throws exception for nonexistent file
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip 5.2 seems to get a different exception"); ?>
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();

try {
    $gridfs->put('/does/not/exist');
    var_dump(false);
} catch (MongoGridFSException $e) {
    var_dump($e->getMessage(), $e->getCode());
}
--EXPECT--
string(38) "error setting up file: /does/not/exist"
int(7)
