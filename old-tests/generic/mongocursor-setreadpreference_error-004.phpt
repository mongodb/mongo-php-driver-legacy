--TEST--
MongoCursor::setReadPreference() error changing read preference mode to primary with tag sets
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$m = new_mongo_standalone();
$c = $m->phpunit->test->find();
$c->setReadPreference(MongoClient::RP_SECONDARY, array( array( 'foo' => 'bar' ) ) );
$c->setReadPreference(MongoClient::RP_PRIMARY, array( array( 'foo' => 'bar' ) ) );
$rp = $c->getReadPreference();
var_dump($rp);
?>
--EXPECTF--
Warning: MongoCursor::setReadPreference(): You can't use read preference tags with a read preference of PRIMARY in %s on line %d
array(2) {
  ["type"]=>
  string(9) "secondary"
  ["tagsets"]=>
  array(1) {
    [0]=>
    array(1) {
      ["foo"]=>
      string(3) "bar"
    }
  }
}
