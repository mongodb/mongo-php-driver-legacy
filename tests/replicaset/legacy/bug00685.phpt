--TEST--
Test for PHP-685: wtimeout option is not supported per-query
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = new_mongo();

try {
    $m->selectDb(dbname())->test->insert(array("random" => "data"), array("wtimeout" => 1, "w" => 7));
} catch(MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--EXPECTF--
string(%s) "%s:%d: timeout"
int(4)
