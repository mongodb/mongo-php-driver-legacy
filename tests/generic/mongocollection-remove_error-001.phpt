--TEST--
MongoCollection::remove() error with non-UTF8 strings
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection(dbname(), 'remove');
$coll->drop();

try {
    $coll->remove(array('foo' => "\xFE\xF0"));
} catch (Exception $e) {
    printf("%s: %d\n", get_class($e), $e->getCode());
}
?>
--EXPECT--
MongoException: 12
