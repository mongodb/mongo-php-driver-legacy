--TEST--
MongoCommandCursor rewind
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();
$dbname = dbname();

$m = new MongoClient($dsn);
$d = $m->selectDB($dbname);
$d->cursorcmd->drop();

for ($i = 0; $i < 10; $i++) {
	$d->cursorcmd->insert(array('article_id' => $i));
}

$c = new MongoCommandCursor(
	$m, "{$dbname}.cursorcmd",
	array( 'aggregate' => 'cursorcmd', 'cursor' => array('batchSize' => 0 ))
);
$r = $c->rewind();
var_dump($r);

$c = new MongoCommandCursor(
	$m, "{$dbname}.cursorcmd",
	array( 'aggregate' => 'cursorcmd', 'cursor' => array('batchSize' => 2 ))
);
$r = $c->rewind();
var_dump($r['cursor']['firstBatch']);
?>
--EXPECTF--
array(2) {
  ["cursor"]=>
  array(3) {
    ["id"]=>
    object(MongoInt64)#%d (1) {
      ["value"]=>
      string(%d) "%d"
    }
    ["ns"]=>
    string(14) "test.cursorcmd"
    ["firstBatch"]=>
    array(0) {
    }
  }
  ["ok"]=>
  float(1)
}
array(2) {
  [0]=>
  array(2) {
    ["_id"]=>
    object(MongoId)#%d (1) {
      ["$id"]=>
      string(24) "%s"
    }
    ["article_id"]=>
    int(0)
  }
  [1]=>
  array(2) {
    ["_id"]=>
    object(MongoId)#%d (1) {
      ["$id"]=>
      string(24) "%s"
    }
    ["article_id"]=>
    int(1)
  }
}
