--TEST--
MongoCollection::insert() error with excessive document size
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection(dbname(), 'insert');
$coll->drop();

try {
    $coll->insert(array('str' => str_repeat('a', 16 * 1024 * 1024)));
} catch (Exception $e) {
    printf("%s: %d\n", get_class($e), $e->getCode());
}
?>
--EXPECT--
MongoException: 3
