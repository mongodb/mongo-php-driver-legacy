--TEST--
MongoCommandCursor integer sizes
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
<?php if (8 !== PHP_INT_SIZE) { die('skip Only for 64-bit platform'); } ?>
--FILE--
<?php
require "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();
$dbname = dbname();

$m = new MongoClient($dsn);
$d = $m->selectDB($dbname);
$d->cursorcmd->drop();

for ($i = 0; $i < 10; $i++) {
	$d->cursorcmd->insert( array( 
		'int' => $i, 
		'i32' => new MongoInt32( $i ), 
		'i64' => new MongoInt64( $i ) 
	) );
}

function run_command($limit = 2, $batchSize = 1)
{
	global $m, $dbname;

	$c = new MongoCommandCursor(
		$m, "{$dbname}.cursorcmd",
		array(
			'aggregate' => 'cursorcmd', 
			'pipeline' => array( 
				array( '$limit' => $limit ), 
				array( '$sort' => array( 'article_id' => 1 ) ) 
			), 
			'cursor' => array( 'batchSize' => $batchSize )
		)
	);
	return $c;
}

function do_test($limit = 2, $batchSize = 1)
{
	$c = run_command($limit, $batchSize);

	foreach ($c as $key => $record) {
		unset( $record['_id'] );
		var_dump($record);
	}
	echo "====\n";
}

ini_set('mongo.native_long', 0);
ini_set('mongo.long_as_object', 0);
do_test();

ini_set('mongo.long_as_object', 1);
do_test();

ini_set('mongo.native_long', 1);
ini_set('mongo.long_as_object', 0);
do_test();

ini_set('mongo.long_as_object', 1);
do_test();

?>
--EXPECTF--
array(3) {
  ["int"]=>
  float(0)
  ["i32"]=>
  int(0)
  ["i64"]=>
  float(0)
}
array(3) {
  ["int"]=>
  float(1)
  ["i32"]=>
  int(1)
  ["i64"]=>
  float(1)
}
====
array(3) {
  ["int"]=>
  object(MongoInt64)#9 (1) {
    ["value"]=>
    string(1) "0"
  }
  ["i32"]=>
  int(0)
  ["i64"]=>
  object(MongoInt64)#10 (1) {
    ["value"]=>
    string(1) "0"
  }
}
array(3) {
  ["int"]=>
  object(MongoInt64)#5 (1) {
    ["value"]=>
    string(1) "1"
  }
  ["i32"]=>
  int(1)
  ["i64"]=>
  object(MongoInt64)#8 (1) {
    ["value"]=>
    string(1) "1"
  }
}
====
array(3) {
  ["int"]=>
  int(0)
  ["i32"]=>
  int(0)
  ["i64"]=>
  int(0)
}
array(3) {
  ["int"]=>
  int(1)
  ["i32"]=>
  int(1)
  ["i64"]=>
  int(1)
}
====
array(3) {
  ["int"]=>
  object(MongoInt64)#9 (1) {
    ["value"]=>
    string(1) "0"
  }
  ["i32"]=>
  int(0)
  ["i64"]=>
  object(MongoInt64)#7 (1) {
    ["value"]=>
    string(1) "0"
  }
}
array(3) {
  ["int"]=>
  object(MongoInt64)#3 (1) {
    ["value"]=>
    string(1) "1"
  }
  ["i32"]=>
  int(1)
  ["i64"]=>
  object(MongoInt64)#10 (1) {
    ["value"]=>
    string(1) "1"
  }
}
====
