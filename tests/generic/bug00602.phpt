--TEST--
No longer possible to get field information from $cursor->info()
--SKIPIF--
<?php require_once __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";


$m = mongo();
$cursor = $m->selectDb(dbname())->bug602->find()->skip(3)->limit(1);
var_dump($cursor->info());
$cursor->getNext();
var_dump($cursor->info());
?>
--EXPECTF--
array(8) {
  ["ns"]=>
  string(%d) "%s.bug602"
  ["limit"]=>
  int(1)
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
  int(1)
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
  int(0)
  ["at"]=>
  int(0)
  ["numReturned"]=>
  int(0)
  ["server"]=>
  string(%d) "%s"
}
