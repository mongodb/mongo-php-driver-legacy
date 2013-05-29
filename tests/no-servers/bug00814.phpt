--TEST--
Test for bug PHP-814: Passing in invalid MongoDB to MongoDBRef::get() segfaults
--FILE--
<?php

class MyDB extends MongoDB {
	public function __construct() {}
}

$db = new MyDB;

try {
	MongoDBRef::get($db, array('$ref' => "", '$id' => 1));
} catch (MongoException $e) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}
?>
--EXPECT--
int(0)
string(72) "The MongoDB object has not been correctly initialized by its constructor"
