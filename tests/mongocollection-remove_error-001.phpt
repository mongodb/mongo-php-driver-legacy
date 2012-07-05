--TEST--
MongoCollection::remove() error with non-UTF8 strings
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'remove');
$coll->drop();

try {
    $coll->remove(array('foo' => "\xFE\xF0"));
} catch (Exception $e) {
    printf("%s: %d\n", get_class($e), $e->getCode());
}
?>
--EXPECT--
MongoException: 12
