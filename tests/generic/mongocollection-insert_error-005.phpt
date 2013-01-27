--TEST--
MongoCollection::insert() error with dot characters in keys
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection(dbname(), 'insert');
$coll->drop();

try {
	$coll->insert(array('x.y' => 'z'));
} catch (Exception $e) {
	printf("%s: %d\n", get_class($e), $e->getCode());
}
?>
--EXPECT--
MongoException: 2
