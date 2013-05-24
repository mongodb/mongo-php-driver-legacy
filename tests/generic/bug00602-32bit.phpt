--TEST--
Test for PHP-602: No longer possible to get field information from $cursor->info().
--SKIPIF--
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
ini_set('mongo.long_as_object', 1);

$m = mongo_standalone();
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
array(15) {
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
  object(MongoInt64)#%d (1) {
    ["value"]=>
    string(%d) "%d"
  }
  ["at"]=>
  int(1)
  ["numReturned"]=>
  int(2)
  ["server"]=>
  string(%d) "%s"
  ["host"]=>
  string(%d) "%s"
  ["port"]=>
  int(%d)
  ["connection_type_desc"]=>
  string(%d) "%s"
}
