--TEST--
Test for PHP-412: Connecting to replicaset with authentication without providing any credentials
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
// Intentionally only not specifying credentials or the replicaset option
$m = new Mongo($REPLICASET_AUTH_PRIMARY);
var_dump($m);
try {
    $m->phpunit->collection->findOne();
} catch(MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}
?>
--EXPECTF--
object(Mongo)#%d (%d) {
  ["connected"]=>
  bool(true)
  ["status"]=>
  NULL
  ["server":protected]=>
  NULL
  ["persistent":protected]=>
  NULL
}
string(%d) "%s:%d: unauthorized db:%s ns:%s.collection lock type:0 client:%s"
int(10057)

