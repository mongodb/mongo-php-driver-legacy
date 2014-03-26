--TEST--
MongoCommandCursor iteration [2] (limit=5, batchSize=2)
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
	array(
		'aggregate' => 'cursorcmd', 
		'pipeline' => array( 
			array( '$limit' => 5 ), 
			array( '$sort' => array( 'article_id' => 1 ) ) 
		), 
		'cursor' => array( 'batchSize' => 2 )
	)
);

foreach ($c as $key => $record) {
	var_dump($key);
	var_dump($record);
}
?>
--EXPECTF--
int(0)
array(2) {
  ["_id"]=>
  object(MongoId)#8 (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(0)
}
int(1)
array(2) {
  ["_id"]=>
  object(MongoId)#9 (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(1)
}
int(2)
array(2) {
  ["_id"]=>
  object(MongoId)#8 (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(2)
}
int(3)
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(3)
}
int(4)
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(4)
}
