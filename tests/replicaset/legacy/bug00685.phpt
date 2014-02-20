--TEST--
Test for PHP-685: wtimeout option is not supported per-query
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = new_mongo();

$start = time();

try {
    $m->selectDb(dbname())->test->insert(array("random" => "data"), array("wtimeout" => 1, "w" => 7));
} catch(MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

if ((time() - $start) > 2) {
	echo "timeout longer than it should have been\n";
}
?>
--EXPECTF--
%s: MongoCollection::insert(): The 'wtimeout' option is deprecated, please use 'wTimeoutMS' instead in %sbug00685.php on line %d
string(%d) "%s:%d:%stime%S"
int(%d)
