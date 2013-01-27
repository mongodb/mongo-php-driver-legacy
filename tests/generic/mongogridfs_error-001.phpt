--TEST--
MongoGridFS constructor with invalid prefix
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

try {
		$gridfs = new MongoGridFS($db, null);
		var_dump(false);
} catch (Exception $e) {
		var_dump($e->getMessage(), $e->getCode());
}
--EXPECT--
string(42) "MongoGridFS::__construct(): invalid prefix"
int(2)
