--TEST--
Bug#339 (Segfault on insert timeout)
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
require_once __DIR__ ."/../utils.inc";
$m = mongo();
$c = $m->selectDB(dbname())->selectCollection("collection");

try {
    $foo = array("foo" => time());
    $result = $c->insert($foo, array("safe" => true, "timeout" => 1));
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage());
    var_dump($foo);
}
?>
===DONE===
--EXPECTF--
string(27) "MongoCursorTimeoutException"
string(%d) "cursor timed out (timeout: 1, time left: 0:1000, status: 0)"
array(2) {
  ["foo"]=>
  int(%d)
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
===DONE===

