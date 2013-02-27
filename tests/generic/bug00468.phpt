--TEST--
Test for PHP-468: Undefined behavior calling MongoGridFSFile::write() without a filename.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php

require_once "tests/utils/server.inc";

$m = mongo_standalone();
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

