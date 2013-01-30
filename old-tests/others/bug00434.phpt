--TEST--
Test for PHP-434: Mongo::connect() doesn't validate the object.
--FILE--
<?php
class m extends Mongo { function __construct() {} }
try {
    $m = new m;
    $m->connect();
} catch(Exception $e) {
    var_dump($e->getMessage());
}
?>
===DONE===
--EXPECT--
string(70) "The Mongo object has not been correctly initialized by its constructor"
===DONE===


