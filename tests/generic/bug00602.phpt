--TEST--
Test for PHP-602: No longer possible to get field information from $cursor->info().
--SKIPIF--
<?php require_once __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";

$m = mongo();
$c = $m->selectDb(dbname())->bug602;
$c->remove();
$c->insert( array( 'test' => 'one' ) );
$c->insert( array( 'test' => 'two' ) );
$c->insert( array( 'test' => 'three' ) );
$c->insert( array( 'test' => 'four' ) );
$c->insert( array( 'test' => 'five' ) );
$c->insert( array( 'test' => 'six' ) );
$c->insert( array( 'test' => 'seven' ) );
$cursor = $c->find()->skip(3)->limit(2);
var_dump($cursor->info());
$cursor->getNext();
var_dump($cursor->info());
?>
--EXPECTF--
array(8) {
  ["ns"]=>
  string(%d) "%s.bug602"
  ["limit"]=>
  int(2)
  ["batchSize"]=>
  int(0)
  ["skip"]=>
  int(3)
  ["flags"]=>
  int(0)
  ["query"]=>
  object(stdClass)#%d (0) {
  }
  ["fields"]=>
  object(stdClass)#%d (0) {
  }
  ["started_iterating"]=>
  bool(false)
}
array(12) {
  ["ns"]=>
  string(%d) "%s.bug602"
  ["limit"]=>
  int(2)
  ["batchSize"]=>
  int(0)
  ["skip"]=>
  int(3)
  ["flags"]=>
  int(0)
  ["query"]=>
  object(stdClass)#%d (0) {
  }
  ["fields"]=>
  object(stdClass)#%d (0) {
  }
  ["started_iterating"]=>
  bool(true)
  ["id"]=>
  int(%d)
  ["at"]=>
  int(1)
  ["numReturned"]=>
  int(2)
  ["server"]=>
  string(%d) "%s"
}
