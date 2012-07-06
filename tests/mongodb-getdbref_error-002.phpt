--TEST--
MongoDB::getDBRef() throws exception if $ref or $db fields are not strings
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$db = $mongo->selectDB('test');

try {
    $db->getDBRef(array('$ref' => 1, '$id' => 2));
} catch (Exception $e) {
    printf("%s: %s\n", get_class($e), $e->getCode());
}

try {
    $db->getDBRef(array('$ref' => 'dbref', '$id' => 2, '$db' => 3));
} catch (Exception $e) {
    printf("%s: %d\n", get_class($e), $e->getCode());
}
?>
--EXPECT--
MongoException: 10
MongoException: 11
