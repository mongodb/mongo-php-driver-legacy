--TEST--
MongoCommandCursor (iterating twice, after createFromDocument)
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

$res = $d->command( 
	array(
		'aggregate' => 'cursorcmd', 
		'pipeline' => array( 
			array( '$limit' => 7 ) 
		), 
		'cursor' => array( 'batchSize' => 3 )
	),
	null,
	$hash
);

$c = MongoCommandCursor::createFromDocument($m, $hash, $res);
$c->batchSize(3);

foreach( $c as $res )
{
	echo $res['article_id'], ' ';
}
echo "\n";
try {
	foreach( $c as $res )
	{
		echo $res['article_id'], ' ';
	}
	echo "\n";
} catch (MongoCursorException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}
?>
--EXPECT--
0 1 2 3 4 5 6 
33
cannot iterate twice with command cursors created through createFromDocument
