--TEST--
MongoCommandCursor (wrong namespaces)
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

// TEST with invalid ns
$c = new MongoCommandCursor(
	$m, "unknown ns",
	array(
		'aggregate' => 'cursorcmd',
		'pipeline' => array(
			array( '$limit' => 5 ),
			array( '$sort' => array( 'article_id' => 1 ) )
		),
		'cursor' => array( 'batchSize' => 2 )
	)
);

try {
	$c->rewind();
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}

// TEST with unknown ns
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

try {
	foreach ($c as $key => $result) {
		var_dump($key, $result);
	}
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
MongoDB::__construct(): invalid name unknown ns
string(24) "5%s"
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(0)
}
string(24) "5%s"
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(1)
}
string(24) "5%s"
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(2)
}
string(24) "5%s"
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(3)
}
string(24) "5%s"
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(4)
}
