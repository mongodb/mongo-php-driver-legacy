--TEST--
MongoCollection::insert() error with dot characters in keys
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection('test', 'insert');
$coll->drop();

try {
    $coll->insert(array('x.y' => 'z'));
} catch (Exception $e) {
    printf("%s: %d\n", get_class($e), $e->getCode());
}
?>
--EXPECT--
MongoException: 2
