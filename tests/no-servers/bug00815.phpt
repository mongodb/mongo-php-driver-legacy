--TEST--
Test for PHP-815: MongoCursor ctor doesn't validate the MongoClient object.
--FILE--
<?php
class MyMongoClient extends MongoClient {
    public function __construct() {
    }
}
class MyDB extends MongoDB {
    public function __construct() {}
}

$db = new MyDB;

try {
	$c = new MongoCursor(new MyMongoClient, "foo.test");
} catch (MongoException $e) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}
?>
==DONE==
--EXPECT--
int(0)
string(76) "The MongoClient object has not been correctly initialized by its constructor"
==DONE==
