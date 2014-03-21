--TEST--
MongoCommandCursor: error in command
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();
$dbname = dbname();

$m = new MongoClient($dsn);
$d = $m->selectDB($dbname);
$d->cursorcmd->drop();

for ($i = 0; $i < 10; $i++) {
	$d->cursorcmd->insert(array('article_id' => $i));
}

try {
$document = $d->cursorcmd->aggregate(
	array( array( '$limit' => 4 ), array( '$sort' => 1 ) ), array('cursor' => array('batchSize' => 2 ))
);
	foreach ($c as $key => $record) {
		var_dump($key);
		var_dump($record);
	}
} catch ( MongoResultException $e ) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
15973
%s:%d: exception:  the $sort key specification must be an object
