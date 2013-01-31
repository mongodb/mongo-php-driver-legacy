--TEST--
MongoDBRef::get() throws exception if $ref or $db fields are not strings
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB('test');

try {
    MongoDBRef::get($db, array('$ref' => 1, '$id' => 2));
} catch (Exception $e) {
    printf("%s: %s\n", get_class($e), $e->getCode());
}

try {
    MongoDBRef::get($db, array('$ref' => 'dbref', '$id' => 2, '$db' => 3));
} catch (Exception $e) {
    printf("%s: %d\n", get_class($e), $e->getCode());
}
?>
--EXPECT--
MongoException: 10
MongoException: 11
