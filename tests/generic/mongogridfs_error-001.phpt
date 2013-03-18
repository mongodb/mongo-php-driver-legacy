--TEST--
MongoGridFS constructor with invalid prefix
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

try {
    $gridfs = new MongoGridFS($db, null);
    var_dump(false);
} catch (Exception $e) {
    var_dump($e->getMessage(), $e->getCode());
}
--EXPECT--
string(42) "MongoGridFS::__construct(): invalid prefix"
int(2)
