--TEST--
Test for PHP-339: Segfault on insert timeout. (no streams)
--SKIPIF--
<?php if (getenv('SKIP_SLOW_TESTS')) die('skip slow tests excluded by request'); ?>
<?php if (MONGO_STREAMS) { die("skip This test requires streams support to be disabled"); } ?>
<?php require_once "tests/utils/bridge.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getBridgeInfo();
$m = new MongoClient($dsn, array('connectTimeoutMS' => 2000, 'socketTimeoutMS' => 2000));
$c = $m->selectDB(dbname())->selectCollection("collection");

try {
    $foo = array("foo" => time());
    $result = $c->insert($foo, array("safe" => true, "timeout" => 1));
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage(), $e->getCode());
    var_dump($foo);
}
try {
    $foo = array("foo" => "bar");
    $c->insert($foo, array("safe" => true));
    $result = $c->findOne(array("_id" => $foo["_id"]));
    var_dump($result);
} catch(Exception $e) {
    printf("FAILED %s: %s\n", get_class($e), $e->getMessage(), $e->getCode());
}
?>
===DONE===
--EXPECTF--
string(27) "MongoCursorTimeoutException"
string(%d) "%s:%d: Timed out waiting for header data"
int(80)
array(2) {
  ["foo"]=>
  int(%d)
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["foo"]=>
  string(%d) "bar"
}
===DONE===

