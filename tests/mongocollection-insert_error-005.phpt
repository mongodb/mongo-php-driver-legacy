--TEST--
MongoCollection::insert() error with dot characters in keys
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
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
