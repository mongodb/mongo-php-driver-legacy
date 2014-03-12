--TEST--
MongoCollection::commandCursor (broken cursor/batchsize structure)
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

$c = $d->cursorcmd;

$r = $c->commandCursor(
	array(
		'aggregate' => 'cursorcmd', 'pipeline' => array( array( '$limit' => 2 ) ),
		'cursor' => 42
	)
);
try {
	foreach ($r as $key => $record) {}
} catch (MongoCursorException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}

?>
--EXPECT--
32
The cursor command's 'cursor' element is not an array
