--TEST--
MongoDB::cursorCommand() with batchSize()
--SKIPIF--
<?php $needs = "2.5.1"; require "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();

function log_getmore($server, $cursor_options)
{
	echo __METHOD__, "\n";
}


$ctx = stream_context_create(
	array(
		"mongodb" => array( "log_getmore" => "log_getmore",)
	)
);

$m = new MongoClient($dsn, array(), array("context" => $ctx));

$d = $m->selectDB(dbname());
$d->cursorcmd->drop();

$p = str_repeat("0123456789", 128);

for ($i = 0; $i < 15; $i++) {
	$d->cursorcmd->insert( array( 'article_id' => $i, 'pad' => $p ) );
}

$cursor = $d->cursorCommand(array(
	'aggregate' => 'cursorcmd', 
	'pipeline' => array( 
		array( '$match' => array( 'article_id' => array( '$gte' => 2 ) ) )
	)
));

$cursor->batchSize(4);
while($r = $cursor->getNext()) {
	echo $r['article_id'], "\n";
}

?>
--EXPECTF--
log_getmore
2
3
4
5
log_getmore
6
7
8
9
log_getmore
10
11
12
13
log_getmore
14
