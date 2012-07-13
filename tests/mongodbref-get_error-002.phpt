--TEST--
MongoDBRef::get() throws exception if $ref or $db fields are not strings
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
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
