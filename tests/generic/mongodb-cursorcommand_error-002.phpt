--TEST--
MongoDB::cursorCommand() with twice iteration
--SKIPIF--
<?php $needs = "2.5.1"; require "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($dsn);
$d = $m->selectDB(dbname());
$d->cursorcmd->drop();

$p = str_repeat("0123456789", 128);

for ($i = 0; $i < 5; $i++) {
	$d->cursorcmd->insert( array( 'article_id' => $i, 'pad' => $p ) );
}

$cursor = $d->cursorCommand(array(
	'aggregate' => 'cursorcmd', 
	'pipeline' => array( 
		array( '$match' => array( 'article_id' => array( '$gte' => 2 ) ) )
	)
));

foreach ($cursor as $record) { }
try { 
	foreach ($cursor as $record) { }
} catch (MongoCursorException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
30
Can't reset command cursors
