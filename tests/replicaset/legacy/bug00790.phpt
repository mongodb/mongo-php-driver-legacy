--TEST--
Test for PHP-790: MongoCollection write operation wtimeout option inconsistent with connection string wTimeoutMS option
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = new_mongo();

$start = time();

try {
    $m->selectDb(dbname())->test->insert(array("random" => "data"), array("wTimeoutMS" => 1, "w" => 7));
} catch(MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

if ((time() - $start) > 2) {
	echo "timeout longer than it should have been\n";
}
?>
--EXPECTF--
string(%d) "%s:%d:%stime%S"
int(%d)
