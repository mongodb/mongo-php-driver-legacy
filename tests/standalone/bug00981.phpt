--TEST--
Test for PHP-981: Empty document should not throw exception
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
$c = $m->selectCollection(dbname(), "bug981");
$c->drop();

$retval = $c->insert(array());
dump_these_keys($retval, array("ok"));

$a = array();
$retval = $c->insert($a);
dump_these_keys($retval, array("ok"));
var_dump($a);

$retval = $c->save(array());
dump_these_keys($retval, array("ok"));

echo "DONE\n";
?>
--EXPECTF--
array(1) {
  ["ok"]=>
  float(1)
}
array(1) {
  ["ok"]=>
  float(1)
}
array(1) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
array(1) {
  ["ok"]=>
  float(1)
}
DONE
