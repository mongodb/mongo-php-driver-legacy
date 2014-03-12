--TEST--
MongoCommandCursor::info()
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
			array( '$limit' => 7 ) 
		), 
		'cursor' => array( 'batchSize' => 3 )
	)
);

$c->rewind();
while ($c->valid()) {
	$key = $c->key();
	$record = $c->current();

	$i = $c->info();
	var_dump($i['at'], $i['numReturned'], $i['firstBatchAt'], $i['firstBatchNumReturned']);
	echo "===\n";
	$c->next();
}
echo "End of iteration:\n";
$i = $c->info();
var_dump($i['at'], $i['numReturned'], $i['firstBatchAt'], $i['firstBatchNumReturned']);
?>
--EXPECT--
int(0)
int(0)
int(0)
int(3)
===
int(0)
int(0)
int(1)
int(3)
===
int(0)
int(0)
int(2)
int(3)
===
int(0)
int(3)
int(3)
int(3)
===
int(1)
int(3)
int(3)
int(3)
===
int(2)
int(3)
int(3)
int(3)
===
int(3)
int(4)
int(3)
int(3)
===
End of iteration:
int(4)
int(4)
int(3)
int(3)
