--TEST--
Test for PHP-468: Undefined behavior calling MongoGridFSFile::write() without a filename.
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php

require_once dirname(__FILE__) . "/../utils.inc";

$m = mongo();
$grid = $m->selectDB(dbname())->getGridFS();
$id = $grid->storeBytes('foo');
$file = $grid->get($id);
try {
	$file->write();
} catch(MongoGridFSException $e) {
	var_dump($e->getMessage(), $e->getCode());
}
?>
--EXPECTF--
string(20) "Cannot find filename"
int(15)

