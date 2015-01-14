--TEST--
MongoCollection::createIndex() error if namespace exceeds 127 bytes (legacy)
--SKIPIF--
<?php $needs = "2.5.5"; $needsOp = "<"; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$c = $mc->selectCollection(dbname(), collname(__FILE__));

try {
    $c->ensureIndex(
        array('x' => 1),
        array('name' => str_repeat('a', 200))
    );
} catch (MongoException $e) {
    printf("error message: %s\n", $e->getMessage());
}

?>
===DONE===
--EXPECTF--
error message: %sname too long%s
===DONE===
