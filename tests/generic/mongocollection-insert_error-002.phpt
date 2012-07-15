--TEST--
MongoCollection::insert() error with non-UTF8 strings
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection('test', 'insert');
$coll->drop();

$documents = array(
    array('_id' => "\xFE\xF0"),
    array('x' => "\xFE\xF0"),
    (object) array('x' => "\xFE\xF0"),
    array('x' => new MongoCode('return y;', array('y' => "\xFE\xF0"))),
);

foreach ($documents as $document) {
    try {
        $coll->insert($document);
    } catch (Exception $e) {
        printf("%s: %d\n", get_class($e), $e->getCode());
    }
}
?>
--EXPECT--
MongoException: 12
MongoException: 12
MongoException: 12
MongoException: 12
