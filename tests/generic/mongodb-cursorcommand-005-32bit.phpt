--TEST--
MongoCommandCursor integer sizes
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
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

/* Expected to fail, because the first batch contains int64s */
try {
	$c = run_command();
	$c->doQuery();
	$c->next();
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}
echo "====\n";

/* Now doing it in such a way that the first batch doesn't contain any
 * documents. This should work. */
$c = run_command(3, 0);
var_dump($c->doQuery());
ini_set('mongo.long_as_object', 1);
$c->batchSize(1);
$c->valid();
$c->next();
var_dump($c->current());
$c->valid();
$c->next();
var_dump($c->current());
echo "====\n";

ini_set('mongo.long_as_object', 1);
do_test();

?>
--EXPECTF--
array(3) {
  ["int"]=>
  int(0)
  ["i32"]=>
  int(0)
  ["i64"]=>
  float(0)
}
array(3) {
  ["int"]=>
  int(1)
  ["i32"]=>
  int(1)
  ["i64"]=>
  float(1)
}
====
array(3) {
  ["int"]=>
  int(0)
  ["i32"]=>
  int(0)
  ["i64"]=>
  object(MongoInt64)#9 (1) {
    ["value"]=>
    string(1) "0"
  }
}
array(3) {
  ["int"]=>
  int(1)
  ["i32"]=>
  int(1)
  ["i64"]=>
  object(MongoInt64)#5 (1) {
    ["value"]=>
    string(1) "1"
  }
}
====
Unknown error executing command (empty document returned)
====
array(2) {
  ["cursor"]=>
  array(3) {
    ["id"]=>
    object(MongoInt64)#9 (1) {
      ["value"]=>
      string(18) "%s"
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
array(4) {
  ["_id"]=>
  object(MongoId)#9 (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["int"]=>
  int(0)
  ["i32"]=>
  int(0)
  ["i64"]=>
  object(MongoInt64)#5 (1) {
    ["value"]=>
    string(1) "0"
  }
}
array(4) {
  ["_id"]=>
  object(MongoId)#5 (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["int"]=>
  int(1)
  ["i32"]=>
  int(1)
  ["i64"]=>
  object(MongoInt64)#9 (1) {
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
  object(MongoInt64)#14 (1) {
    ["value"]=>
    string(1) "0"
  }
}
array(3) {
  ["int"]=>
  int(1)
  ["i32"]=>
  int(1)
  ["i64"]=>
  object(MongoInt64)#12 (1) {
    ["value"]=>
    string(1) "1"
  }
}
====
