--TEST--
MongoCollection::insert() error with excessive document size
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'insert');
$coll->drop();

try {
    $coll->insert(array('str' => str_repeat('a', 16 * 1024 * 1024)));
} catch (Exception $e) {
    printf("%s: %d\n", get_class($e), $e->getCode());
}
?>
--EXPECT--
MongoException: 3
