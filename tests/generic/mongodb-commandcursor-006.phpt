--TEST--
MongoCommandCursor::dead()
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
			array( '$limit' => 5 ) 
		), 
		'cursor' => array( 'batchSize' => 2 )
	)
);

$c->rewind();
while ($c->valid()) {
	$key = $c->key();
	$record = $c->current();

	var_dump($c->dead());
	$c->next();
}
echo "No longer valid:\n";
var_dump($c->dead());
?>
--EXPECT--
bool(false)
bool(false)
bool(false)
bool(false)
bool(true)
No longer valid:
bool(true)
