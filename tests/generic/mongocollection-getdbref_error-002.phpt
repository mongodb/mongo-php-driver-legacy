--TEST--
MongoCollection::getDBRef() throws exception if $ref or $db fields are not strings
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection(dbname(), 'dbref');

try {
    $coll->getDBRef(array('$ref' => 1, '$id' => 2));
} catch (Exception $e) {
    printf("%s: %s\n", get_class($e), $e->getCode());
}

try {
    $coll->getDBRef(array('$ref' => 'dbref', '$id' => 2, '$db' => 3));
} catch (Exception $e) {
    printf("%s: %d\n", get_class($e), $e->getCode());
}
?>
--EXPECT--
MongoException: 10
MongoException: 11
