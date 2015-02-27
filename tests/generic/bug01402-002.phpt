--TEST--
Test for PHP-1402: hasNext() returns false but getNext() returns result for limit=1
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$c->save(array('_id' => 1));
$c->save(array('_id' => 2));

$cur = $c->find()->limit(1);

echo "Checking first result:\n";
var_dump($cur->hasNext());
var_dump($cur->getNext());

echo "\nChecking second result:\n";
var_dump($cur->hasNext());
var_dump($cur->getNext());

?>
===DONE===
--EXPECT--
Checking first result:
bool(true)
array(1) {
  ["_id"]=>
  int(1)
}

Checking second result:
bool(false)
NULL
===DONE===
