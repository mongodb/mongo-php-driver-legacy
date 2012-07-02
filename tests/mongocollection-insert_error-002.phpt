--TEST--
MongoCollection::insert() error with non-UTF8 strings
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'insert');
$coll->drop();

try {
    $coll->insert(array('str' => "\xFE"));
} catch (Exception $e) {
    printf("%s: %d\n", get_class($e), $e->getCode());
}
?>
--EXPECT--
MongoException: 12
