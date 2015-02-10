--TEST--
Test for PHP-1390: hasNext() should return true before advancing to a single result
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

$d = $m->selectDb(dbname());
$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$c->save(array('_id' => 'test1'));

$cursor = $c->find();

var_dump($cursor->hasNext());
var_dump($cursor->getNext());
var_dump($cursor->hasNext());

?>
--EXPECT--
bool(true)
array(1) {
  ["_id"]=>
  string(5) "test1"
}
bool(false)
