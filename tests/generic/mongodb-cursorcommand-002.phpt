--TEST--
MongoCommandCursor iteration [1]
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

for ($i = 0; $i < 500; $i++) {
	$d->cursorcmd->insert(array('article_id' => $i));
}

$c = new MongoCommandCursor(
	$m, "{$dbname}.cursorcmd",
	array( 'aggregate' => 'cursorcmd', 'cursor' => array('batchSize' => 2 ))
);

foreach ($c as $key => $record) {
	var_dump($key);
	var_dump($record);
}
?>
--EXPECTF--
string(24) "5%s"
array(2) {
  ["_id"]=>
  object(MongoId)#8 (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  object(MongoInt64)#9 (1) {
    ["value"]=>
    string(1) "0"
  }
}
string(24) "5%s"
array(2) {
  ["_id"]=>
  object(MongoId)#10 (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  object(MongoInt64)#11 (1) {
    ["value"]=>
    string(1) "1"
  }
}
