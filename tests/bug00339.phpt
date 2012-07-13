--TEST--
Bug#339 (Segfault on insert timeout)
--SKIPIF--
<?php exit("skip move me");
--FILE--
<?php
// This test may fail if you have blazing fast machine and the 1ms timeout
// isn't low enough.

$m = new Mongo("mongodb://user:user@primary.local/phpunit-auth", array("replicaSet" => "foobar"));
$c = $m->selectDB("phpunit-auth")->selectCollection("collection");

try {
    $foo = array("foo" => time());
    $result = $c->insert($foo, array("safe" => true, "timeout" => 1));
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage());
    $result = $c->insert(array( "foo" => time()), array("safe" => true));
    var_dump($foo, $result);
}
?>
===DONE==
--EXPECTF--
string(27) "MongoCursorTimeoutException"
string(59) "cursor timed out (timeout: 1, time left: 0:1000, status: 0)"
array(2) {
  ["foo"]=>
  int(%d)
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
array(5) {
  ["n"]=>
  int(0)
  ["lastOp"]=>
  float(%f)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
===DONE==
