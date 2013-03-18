--TEST--
MongoCollection::insert() error with zero-length keys
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection(dbname(), 'insert');
$coll->drop();

$documents = array(
    array('' => 'foo'),
    array('x' => array('' => 'foo')),
    array('x' => array('' => 'foo'), 'y' => 'z'),
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
MongoException: 1
MongoException: 1
MongoException: 1
